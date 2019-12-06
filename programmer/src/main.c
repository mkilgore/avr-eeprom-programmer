
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "eeprom_tty.h"
#include "msg_handler.h"
#include "arg_parser.h"
#include "cmd.h"

static const char *version_text = "eepgmr-" Q(VER_MAJOR) "." Q(VER_MINOR) "." Q(VER_PATCH);

static const char *arg_str = "[Flags] tty-device";
static const char *usage_str = "Write and read data from an AVR EEPROM Programmer.\n";
static const char *arg_desc_str  = "";

#define XARGS \
    X(read, "read", 'r', 1, "address:length", "Read bytes from EEPROM and display to console") \
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

struct cmd_def {
    enum cmd_type type;
    int (*init) (struct cmd *, const char *);
} cmd_defs[] = {
    [ARG_read] = { .type = CMD_READ, .init = cmd_parse_read },
    [ARG_readfile] = { .type = CMD_READ_FILE, .init = cmd_parse_read_file },
    [ARG_compare] = { .type = CMD_READ_COMPARE_FILE, .init = cmd_parse_read_compare_file },
    [ARG_write] = { .type = CMD_WRITE, .init = cmd_parse_write },
    [ARG_erase_sector] = { .type = CMD_ERASE_SECTOR, .init = cmd_parse_erase_sector },
    [ARG_erase_chip] = { .type = CMD_ERASE_CHIP, .init = NULL },
};

static list_head_t cmd_list = LIST_HEAD_INIT(cmd_list);
static const char *tty_device;
static speed_t baudrate;
static int set_baudrate = 0;

static int cmd_add(enum cmd_type type, int (*init) (struct cmd *, const char *), const char *arg)
{
    struct cmd *cmd = malloc(sizeof(*cmd));
    cmd_init(cmd);

    if (init && init(cmd, arg))
        return 1;

    cmd->type = type;
    list_add_tail(&cmd_list, &cmd->node);
    return 0;
}

int main(int argc, char **argv)
{
    enum arg_index ret;
    struct cmd *cmd;

    while ((ret = arg_parser(argc, argv, ls_args)) != ARG_DONE) {
        switch (ret) {
        case ARG_help:
            display_help_text(argv[0], arg_str, usage_str, arg_desc_str, ls_args);
            return 0;

        case ARG_version:
            printf("%s\n", version_text);
            return 0;

        case ARG_read:
        case ARG_readfile:
        case ARG_compare:
        case ARG_write:
        case ARG_erase_sector:
        case ARG_erase_chip:
            if (cmd_add(cmd_defs[ret].type, cmd_defs[ret].init, argarg))
                return 1;

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

    list_foreach_entry(&cmd_list, cmd, node)
        if (cmd_exec(cmd, &msg_state))
            break;

    eeprom_tty_close(&tty);

    return 0;
}

