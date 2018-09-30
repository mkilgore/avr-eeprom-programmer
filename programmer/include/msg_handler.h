#ifndef INCLUDE_MSG_HANDLER_H
#define INCLUDE_MSG_HANDLER_H

#include <stdint.h>

#include "common_msg.h"

struct msg_handler_state {
    struct eeprom_tty *tty;

    char *line_buffer;
    size_t line_len;
    size_t line_buffer_capacity;
};

enum msg_cmd_response msg_send_erase_sector(struct msg_handler_state *state, uint8_t sector);
enum msg_cmd_response msg_send_erase_chip(struct msg_handler_state *state);
enum msg_cmd_response msg_send_write_block(struct msg_handler_state *state, uint32_t address, const uint8_t *data, uint32_t length);
enum msg_cmd_response msg_send_read_block(struct msg_handler_state *state, uint32_t address, uint8_t *data, uint32_t length);

void msg_reset_stream(struct msg_handler_state *state);

extern const char *msg_cmd_response_strings[];

#endif
