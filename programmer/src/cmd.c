
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "msg_handler.h"
#include "dump_mem.h"
#include "file.h"
#include "progress.h"
#include "cmd.h"

int cmd_parse_read(struct cmd *cmd, const char *address_str)
{
    char *end_ptr = NULL;
    const char *parsed_str = address_str;

    cmd->type = CMD_READ;

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

int cmd_parse_read_file(struct cmd *cmd, const char *arg)
{
    return 1;
}

int cmd_parse_read_compare_file(struct cmd *cmd, const char *arg)
{
    return 1;
}

int cmd_parse_write(struct cmd *cmd, const char *write_str)
{
    char *end_ptr = NULL;
    const char *parsed_str = write_str;

    cmd->type = CMD_WRITE;

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

int cmd_parse_erase_sector(struct cmd *cmd, const char *arg)
{
    char *end_ptr = NULL;

    cmd->type = CMD_ERASE_SECTOR;

    cmd->sector = strtol(arg, &end_ptr, 0);
    if (*end_ptr) {
        printf("Error: Unexpected '%c' in \"%s\"\n", *end_ptr, arg);
        return 1;
    }

    return 0;
}

static void msg_display_response(enum msg_cmd_response response)
{
    if (response != MSG_SUCCESS && response != MSG_SUCCESS_PING)
        printf("Error: %s\n", msg_cmd_response_strings[response]);
}

#define READ_INCREMENT 0x10

static int cmd_exec_read(struct cmd *cmd, struct msg_handler_state *state)
{
    uint8_t *buf = malloc(cmd->length);
    int use_progress = 0;
    enum msg_cmd_response ret;

    uint32_t have_read = 0;

    printf("Read at 0x%08x:\n", cmd->addr);

    if (cmd->length > READ_INCREMENT) {
        use_progress = 1;
        progress_start();
    }

    for (have_read = 0; have_read < cmd->length; have_read += READ_INCREMENT) {
        uint32_t to_read = READ_INCREMENT;
        if (have_read + READ_INCREMENT > cmd->length)
            to_read = cmd->length - have_read;

        ret = msg_send_read_block(state, cmd->addr + have_read, buf + have_read, to_read);
        if (ret)
            break;

        progress_update(((have_read + to_read) * 100) / cmd->length);
    }

    if (use_progress)
        progress_finish();

    if (ret) {
        msg_display_response(ret);
        goto free_buf;
    }

    dump_mem(buf, cmd->length, cmd->addr);

  free_buf:
    free(buf);
    return !!ret;
}

static int get_file_length(int fd)
{
    struct stat stat;
    memset(&stat, 0, sizeof(stat));

    int ret = fstat(fd, &stat);
    if (ret)
        return -1;

    return stat.st_size;
}

static int cmd_exec_write(struct cmd *cmd, struct msg_handler_state *state)
{
    uint8_t buf[(MSG_MAX_LENGTH - 16) / 2];
    uint32_t addr = cmd->addr;
    enum msg_cmd_response ret = MSG_SUCCESS;

    int fd = open_with_dash(cmd->filename, O_RDONLY);
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
    close_with_dash(fd);
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

static int (*cmd_table[]) (struct cmd *, struct msg_handler_state *) = {
    [CMD_READ] = cmd_exec_read,
    [CMD_WRITE] = cmd_exec_write,
    [CMD_ERASE_SECTOR] = cmd_exec_erase_sector,
    [CMD_ERASE_CHIP] = cmd_exec_erase_chip,
};

int cmd_exec(struct cmd *cmd, struct msg_handler_state *state)
{
    int (*func) (struct cmd *, struct msg_handler_state *);

    if (cmd->type >= ARRAY_SIZE(cmd_table))
        return 1;

    func = cmd_table[cmd->type];

    if (!func)
        return 1;

    return (func) (cmd, state);
}
