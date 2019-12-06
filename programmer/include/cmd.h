#ifndef INCLUDE_CMD_H
#define INCLUDE_CMD_H

#include "list.h"
#include "msg_handler.h"

enum cmd_type {
    CMD_READ,
    CMD_READ_FILE,
    CMD_READ_COMPARE_FILE,
    CMD_WRITE,
    CMD_ERASE_SECTOR,
    CMD_ERASE_CHIP,
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

int cmd_parse_read(struct cmd *, const char *arg);
int cmd_parse_read_file(struct cmd *, const char *arg);
int cmd_parse_read_compare_file(struct cmd *, const char *arg);
int cmd_parse_write(struct cmd *, const char *arg);
int cmd_parse_erase_sector(struct cmd *, const char *arg);

int cmd_exec(struct cmd *, struct msg_handler_state *);

#endif
