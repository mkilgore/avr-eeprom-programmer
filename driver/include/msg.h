#ifndef INCLUDE_MSG_H
#define INCLUDE_MSG_H

#include "serial.h"
#include "common_msg.h"

/* All messages start with this byte */
#define MSG_ID 0xAA

enum msg_cmd_id {
    MSG_CMD_READ,
    MSG_CMD_WRITE,
    MSG_CMD_ERASE_SECTOR,
    MSG_CMD_ERASE_CHIP,
};

enum msg_cmd_response {
    MSG_SUCCESS,
    MSG_SUCCESS_PING,

    MSG_ERROR_INVALID_ID,
    MSG_ERROR_OVERFLOW,
    MSG_ERROR_INVALID_CMD,

    MSG_ERROR_READ_INVALID_MSG,

    MSG_ERROR_WRITE_INVALID_MSG,
    MSG_ERROR_WRITE_INVALID_DATA,
    MSG_ERROR_WRITE_FAILURE,

    MSG_ERROR_ERASE_SECTOR_INVALID_MSG,
    MSG_ERROR_ERASE_SECTOR_FAILED,
};

void msg_loop(struct serial *);

#endif
