
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "eeprom_tty.h"
#include "common_msg.h"
#include "msg_handler.h"

const char *msg_cmd_response_strings[] = {
#define X(enm, err_str) [enm] = err_str,
MSG_RESPONSE_INFO_X
#undef X
};

static enum msg_cmd_response msg_read_response(struct msg_handler_state *state)
{
    state->line_len = getline(&state->line_buffer, &state->line_buffer_capacity, state->tty->in_file);

    if (state->line_len <= 2)
        return MSG_ERROR_INVALID_CMD;
    else {
        return hex_to_byte(state->line_buffer +  state->line_len - 3);
    }
}

static void msg_send_command(struct msg_handler_state *state, enum msg_cmd_id id, const char *buf)
{
    FILE *file = state->tty->out_file;
    fputc(MSG_ID, file);
    fputc(id, file);
    fputs(buf, file);
    fputc('\n', file);
    eeprom_tty_flush(state->tty);
}

/* 
 * Resets the stream to a known state by sending an empty command. Any existing
 * command is ended prematurely by this code.
 *
 * The programmer driver only acts on full commands after the newline is sent,
 * so this should never result in the programmer acting on an incomplete
 * command, or terminate a command in progress.
 */
void msg_reset_stream(struct msg_handler_state *state)
{
    do {
        fputc('\n', state->tty->out_file);
        eeprom_tty_flush(state->tty);
    } while (msg_read_response(state) != MSG_SUCCESS_PING);
}

enum msg_cmd_response msg_send_erase_sector(struct msg_handler_state *state, uint8_t sector)
{
    char hex[3];
    byte_to_hex(sector, hex);
    hex[2] = '\0';

    msg_send_command(state, MSG_CMD_ERASE_SECTOR, hex);

    return msg_read_response(state);
}

enum msg_cmd_response msg_send_erase_chip(struct msg_handler_state *state)
{
    char empty[1] = { 0 };

    msg_send_command(state, MSG_CMD_ERASE_CHIP, empty);

    return msg_read_response(state);
}

enum msg_cmd_response msg_send_write_block(struct msg_handler_state *state, uint32_t address, const uint8_t *data, uint32_t length)
{
    if (length * 2> MSG_MAX_LENGTH - 16)
        return 1;

    char hex[16 + length * 2 + 1];
    memset(hex, 0, sizeof(hex));

    uint32_to_hex(address, hex);
    uint32_to_hex(length, hex + 8);

    uint32_t i;
    for (i = 0; i < length; i++)
        byte_to_hex(data[i], hex + 16 + i * 2);

    msg_send_command(state, MSG_CMD_WRITE, hex);

    return msg_read_response(state);
}

enum msg_cmd_response msg_send_read_block(struct msg_handler_state *state, uint32_t address, uint8_t *data, uint32_t length)
{
    char hex[17];
    memset(hex, 0, sizeof(hex));

    uint32_to_hex(address, hex);
    uint32_to_hex(length, hex + 8);

    msg_send_command(state, MSG_CMD_READ, hex);

    enum msg_cmd_response response = msg_read_response(state);

    if (response != MSG_SUCCESS)
        return response;

    uint32_t i;
    for (i = 0; i < length; i++) {
        uint8_t byte = hex_to_byte(state->line_buffer + i * 2);
        data[i] = byte;
    }

    return MSG_SUCCESS;
}


void msg_interactive(struct eeprom_tty *tty)
{
    struct msg_handler_state state = {
        .tty = tty,
        .line_buffer = NULL,
        .line_len = 0,
        .line_buffer_capacity = 0,
    };

    msg_reset_stream(&state);

    free(state.line_buffer);
}

