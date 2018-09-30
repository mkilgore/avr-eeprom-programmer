
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "list.h"
#include "dump_mem.h"
#include "eeprom_tty.h"
#include "msg_handler.h"
#include "arg_parser.h"
#include "progress.h"

static const char *version_text = "eepgmr-" Q(VER_MAJOR) "." Q(VER_MINOR) "." Q(VER_PATCH);

static const char *arg_str = "[Flags] tty-device";
static const char *usage_str = "Write and read data from an AVR EEPROM Programmer.\n";
static const char *arg_desc_str  = "";

#define XARGS \
    X(read, "read", 'r', 1, "address:length", "Read bytes from EEPROM") \
    X(readfile, "read-to-file", 'R', 1, "address:file", "Read bytes from EEPROM to file") \
    X(compare, "compare-to-file", 'c', 1, "address[:length][:offset]:filename", "Compare bytes from EEPROM to file contents") \
    X(write, "write", 'w', 1, "address[:length][:offset]:filename", "Write bytes to EEPROM") \
    X(erase_sector, "erase-sector", 's', 1, "number", "Erase sector from EEPROM") \
    X(erase_chip, "erase-chip", 'e', 0, NULL, "Erase full EEPROM chip") \
    X(baud, "baud", 'b', 1, "baudrate", "Specify the baud rate the programmer communicates at") \
    X(help, "help", 'h', 0, NULL, "Display help") \
    X(version, "version", 'v', 0, NULL, "Display version information") \
    X(last, NULL, '\0', 0, NULL, NULL)

enum arg_index {
    ARG_EXTRA = ARG_PARSER_EXTRA,
    ARG_ERR = ARG_PARSER_ERR,
    ARG_DONE = ARG_PARSER_DONE,
#define X(enu, id, arg, op, arg_text, help_text) ARG_##enu,
    XARGS
#undef X
};

static const struct arg ls_args[] = {
#define X(enu, id, op, arg, arg_text, help_text) CREATE_ARG(enu, id, op, arg, arg_text, help_text)
    XARGS
#undef X
};

struct cmd {
    list_node_t node;

    enum msg_cmd_id type;

    uint32_t addr, length, offset;
    const char *filename;
    uint8_t sector;
};

#define CMD_INIT(cmd) \
    { \
        .node = LIST_NODE_INIT((cmd).node), \
    }

static inline void cmd_init(struct cmd *cmd)
{
    *cmd = (struct cmd)CMD_INIT(*cmd);
}

static list_head_t cmd_list = LIST_HEAD_INIT(cmd_list);
static const char *tty_device;
static speed_t baudrate;
static int set_baudrate = 0;

static int cmd_parse_read(struct cmd *cmd, const char *address_str)
{
    char *end_ptr = NULL;
    const char *parsed_str = address_str;

    cmd->type = MSG_CMD_READ;

    cmd->addr = strtol(parsed_str, &end_ptr, 0);
    if (*end_ptr != ':') {
        if (!*end_ptr) {
            printf("Error: Expected ':' in \"%s\"\n", address_str);
        } else {
            printf("Error: Expected ':', found '%c' in \"%s\"\n", *end_ptr, address_str);
        }
        return 1;
    }

    if (cmd->addr < 0) {
        printf("Error: Address must be positive\n");
        return 1;
    }

    parsed_str = end_ptr + 1;

    cmd->length = strtol(parsed_str, &end_ptr, 0);
    if (*end_ptr) {
        printf("Error: Unexpected character '%c' in \"%s\"", *end_ptr, address_str);
        return 1;
    }

    if (cmd->length < 0) {
        printf("Error: Length must be positive\n");
        return 1;
    }

    return 0;
}

static int cmd_prase_write(struct cmd *cmd, const char *write_str)
{
    char *end_ptr = NULL;
    const char *parsed_str = write_str;

    cmd->type = MSG_CMD_WRITE;

    cmd->addr = strtol(parsed_str, &end_ptr, 0);
    if (*end_ptr != ':') {
        if (!*end_ptr) {
            printf("Error: Expected ':' in \"%s\"\n", write_str);
        } else {
            printf("Error: Expected ':', found '%c' in \"%s\"\n", *end_ptr, write_str);
        }
        return 1;
    }

    parsed_str = end_ptr + 1;

    int i = 0;
    for (i = 0; i < 2; i++) {
        long tmp = strtol(parsed_str, &end_ptr, 0);
        if (*end_ptr != ':') {
            cmd->filename = parsed_str;
            return 0;
        } else {
            if (i == 0) {
                cmd->length = tmp;
            } else if (i == 1) {
                cmd->offset = tmp;
            }
        }
    }

    if (!*parsed_str) {
        printf("Error: Expected filename in \"%s\"\n", write_str);
        return 1;
    }

    cmd->filename = parsed_str;

    return 0;
}

void msg_display_response(enum msg_cmd_response response)
{
    if (response != MSG_SUCCESS && response != MSG_SUCCESS_PING)
        printf("Error: %s\n", msg_cmd_response_strings[response]);
}

static int cmd_exec_read(struct cmd *cmd, struct msg_handler_state *state)
{
    uint8_t *buf = malloc(cmd->length);

    printf("Read at 0x%08x;\n", cmd->addr);
    enum msg_cmd_response ret = msg_send_read_block(state, cmd->addr, buf, cmd->length);

    if (ret) {
        msg_display_response(ret);
        goto free_buf;
    }

    dump_mem(buf, cmd->length, cmd->addr);

  free_buf:
    free(buf);
    return !!ret;
}

int get_file_length(int fd)
{
    int len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return len;
}

static int cmd_exec_write(struct cmd *cmd, struct msg_handler_state *state)
{
    uint8_t buf[(MSG_MAX_LENGTH - 16) / 2];
    uint32_t addr = cmd->addr;
    enum msg_cmd_response ret = MSG_SUCCESS;

    int fd = open(cmd->filename, O_RDONLY);
    int total_len = get_file_length(fd);
    int total_written = 0;
    int len;
    int use_progress = 0, prev_prog = 0;

    printf("Write at 0x%08x: %s\n", addr, cmd->filename);

    if (total_len > sizeof(buf)) {
        use_progress = 1;
        progress_start();
    }


    while ((len = read(fd, buf, sizeof(buf)))) {
        ret = msg_send_write_block(state, addr, buf, len);
        if (ret) {
            if (use_progress)
                progress_finish();

            msg_display_response(ret);
            goto close_fd;
        }

        addr += len;
        total_written += len;

        if (use_progress) {
            int new_prog = (total_written * 100) / total_len;
            if (new_prog > prev_prog)
                progress_update(new_prog);
        }
    }

    if (use_progress)
        progress_finish();

  close_fd:
    close(fd);
    return !!ret;
}

static int cmd_exec_erase_sector(struct cmd *cmd, struct msg_handler_state *state)
{
    printf("Erase Sector: %d\n", cmd->sector);
    enum msg_cmd_response ret = msg_send_erase_sector(state, cmd->sector);
    if (ret)
        msg_display_response(ret);

    return !!ret;
}

static int cmd_exec_erase_chip(struct cmd *cmd, struct msg_handler_state *state)
{
    printf("Erase Chip.\n");
    enum msg_cmd_response ret = msg_send_erase_chip(state);
    if (ret)
        msg_display_response(ret);

    return !!ret;
}

int (*cmd_table[]) (struct cmd *, struct msg_handler_state *) = {
    [MSG_CMD_READ] = cmd_exec_read,
    [MSG_CMD_WRITE] = cmd_exec_write,
    [MSG_CMD_ERASE_SECTOR] = cmd_exec_erase_sector,
    [MSG_CMD_ERASE_CHIP] = cmd_exec_erase_chip,
};

int main(int argc, char **argv)
{
    enum arg_index ret;

    while ((ret = arg_parser(argc, argv, ls_args)) != ARG_DONE) {
        struct cmd *cmd;

        switch (ret) {
        case ARG_help:
            display_help_text(argv[0], arg_str, usage_str, arg_desc_str, ls_args);
            return 0;
        case ARG_version:
            printf("%s\n", version_text);
            return 0;

        case ARG_read:
            cmd = malloc(sizeof(*cmd));
            cmd_init(cmd);
            if (cmd_parse_read(cmd, argarg))
                return 1;

            list_add_tail(&cmd_list, &cmd->node);
            break;

        case ARG_write:
            cmd = malloc(sizeof(*cmd));
            cmd_init(cmd);
            if (cmd_prase_write(cmd, argarg))
                return 1;

            list_add_tail(&cmd_list, &cmd->node);
            break;

        case ARG_erase_sector:
            cmd = malloc(sizeof(*cmd));
            cmd_init(cmd);
            cmd->type = MSG_CMD_ERASE_SECTOR;

            list_add_tail(&cmd_list, &cmd->node);
            break;

        case ARG_erase_chip:
            cmd = malloc(sizeof(*cmd));
            cmd_init(cmd);
            cmd->type = MSG_CMD_ERASE_CHIP;

            list_add_tail(&cmd_list, &cmd->node);
            break;

        case ARG_baud:
            baudrate = atol(argarg);
            set_baudrate = 1;
            break;

        case ARG_EXTRA:
            if (!tty_device) {
                tty_device = argarg;
            } else {
                printf("%s: Unexpected argument '%s'\n", argv[0], argarg);
                return 1;
            }
            break;

        default:
            return 1;
        }
    }

    if (!tty_device) {
        printf("%s: Please specify a TTY device.\n", argv[0]);
        return 1;
    }

    struct eeprom_tty tty;
    memset(&tty, 0, sizeof(tty));

    eeprom_tty_open(&tty, tty_device);

    if (set_baudrate)
        eeprom_tty_set_baud(&tty, baudrate);

    struct msg_handler_state msg_state;
    memset(&msg_state, 0, sizeof(msg_state));

    msg_state.tty = &tty;
    printf("Resetting TTY stream...\n");
    msg_reset_stream(&msg_state);

    struct cmd *cmd;
    list_foreach_entry(&cmd_list, cmd, node) {
        int ret = (cmd_table[cmd->type]) (cmd, &msg_state);
        if (ret)
            break;
    }

    eeprom_tty_close(&tty);

    return 0;
}

