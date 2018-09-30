
#include "common.h"
#include "serial.h"
#include "am29.h"

#include "msg.h"

static void rdy_init(void)
{
    DDRD |= _BV(DDD5);
}

static void rdy_on(void)
{
    sbi(PORTD, PORTD5);
}

static void rdy_off(void)
{
    cbi(PORTD, PORTD5);
}

static uint8_t msg_buf[1024];
static uint16_t msg_len = 0;

/* Format:
 * AAAAAAAALLLLLLLL
 *
 * A: 32-bit address in hex
 * L: 32-bit length in hex
 *
 * Response:
 * XXXX..
 *
 * X: length * 2 number of hex characters
 */
static enum msg_cmd_response handle_read(struct serial *serial, uint8_t *buf, uint16_t len)
{
    if (len != 16)
        return MSG_ERROR_READ_INVALID_MSG;

    uint16_t k;
    for (k = 0; k < len; k++)
        if (!is_hex_char(buf[k]))
            return MSG_ERROR_READ_INVALID_MSG;

    uint32_t addr, read_len;

    addr = hex_to_uint32(buf);
    read_len = hex_to_uint32(buf + 8);

    uint8_t hex[2];

    am29_start();

    uint32_t i;
    for (i = 0; i < read_len; i++) {
        uint8_t byte = am29_read_byte(addr + i);
        byte_to_hex(byte, hex);
        serial_write(serial, hex[0]);
        serial_write(serial, hex[1]);
    }

    am29_finish();

    return MSG_SUCCESS;
}

/*
 * Format:
 * AAAAAAAALLLLLLLLXX..
 *
 * A: 32-bit address in hex
 * L: 32-bit length in hex
 * X: length * 2 number of hex characters
 */
static enum msg_cmd_response handle_write(uint8_t *buf, uint16_t len)
{
    if (len < 17)
        return MSG_ERROR_WRITE_INVALID_MSG;

    /* Message must be a multiple of two, since each byte to write is two chars */
    if (len % 2 != 0)
        return MSG_ERROR_WRITE_INVALID_MSG;

    uint16_t i;
    for (i = 0; i < len; i++)
        if (!is_hex_char(buf[i]))
            return MSG_ERROR_WRITE_INVALID_DATA;

    uint32_t addr, write_len;
    addr = hex_to_uint32(buf);
    write_len = hex_to_uint32(buf + 8);

    if (write_len * 2 != len - 16)
        return MSG_ERROR_WRITE_INVALID_DATA;

    am29_start();

    uint8_t *data = buf + 16;
    int err;

    uint32_t k;
    for (k = 0; k < write_len; k++) {
        uint8_t byte = hex_to_byte(data + k * 2);

        err = am29_write_byte(addr + k, byte);
        if (err)
            break;
    }

    am29_finish();

    if (!err)
        return MSG_SUCCESS;
    else
        return MSG_ERROR_WRITE_FAILURE;
}

/*
 * Format:
 * SS
 *
 * S: 8 bit sector number in hex
 */
static enum msg_cmd_response handle_erase_sector(uint8_t *buf, uint16_t len)
{
    if (len != 2)
        return MSG_ERROR_ERASE_SECTOR_INVALID_MSG;

    if (!is_hex_char(buf[0]) || !is_hex_char(buf[1]))
        return MSG_ERROR_ERASE_SECTOR_INVALID_MSG;

    uint8_t sector = hex_to_byte(buf);
    int err;

    am29_start();
    err = am29_erase_sector(sector);
    am29_finish();

    if (!err)
        return MSG_SUCCESS;
    else
        return MSG_ERROR_ERASE_SECTOR_FAILED;
}

static void msg_finish_response(struct serial *serial, enum msg_cmd_response res)
{
    char hex[2];
    byte_to_hex(res, hex);
    serial_write(serial, hex[0]);
    serial_write(serial, hex[1]);
    serial_write(serial, '\n');
}

void msg_loop(struct serial *serial)
{
    rdy_init();

    while (1) {
        msg_len = 0;

        uint8_t c = 0;

        rdy_on();
        while ((c = serial_read(serial)) != '\n') {
            if (msg_len == sizeof(msg_buf))
                break;

            msg_buf[msg_len++] = c;
        }
        rdy_off();

        if (msg_len == sizeof(msg_buf) && c != '\n') {
            /* The buffer overflowed - loop until we find the \n and then indicate an overflow error and try again */
            while ((c = serial_read(serial)) != '\n')
                ;

            msg_finish_response(serial, MSG_ERROR_OVERFLOW);
            continue;
        }

        if (msg_len == 0) {
            msg_finish_response(serial, MSG_SUCCESS_PING);
            continue;
        }

        if (msg_buf[0] != MSG_ID) {
            msg_finish_response(serial, MSG_ERROR_INVALID_ID);
            continue;
        }

        enum msg_cmd_response response = MSG_ERROR_INVALID_CMD;

        switch (msg_buf[1]) {
        case MSG_CMD_READ:
            response = handle_read(serial, msg_buf + 2, msg_len - 2);
            break;

        case MSG_CMD_WRITE:
            response = handle_write(msg_buf + 2, msg_len - 2);
            break;

        case MSG_CMD_ERASE_SECTOR:
            response = handle_erase_sector(msg_buf + 2, msg_len - 2);
            break;
        }

        msg_finish_response(serial, response);
    }
}

