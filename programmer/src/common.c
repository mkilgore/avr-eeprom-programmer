
#include "common.h"

#include <stdint.h>

static uint8_t hex_to_nibble(uint8_t h)
{
    switch (h) {
    case '0' ... '9':
        return h - '0';

    case 'A' ... 'F':
        return h - 'A' + 10;

    case 'a' ... 'f':
        return h - 'a' + 10;
    }

    return 0;
}

uint8_t hex_to_byte(char *hex)
{
    return (hex_to_nibble(hex[0]) << 4) + hex_to_nibble(hex[1]);
}

static uint8_t nibble_to_hex(uint8_t nib)
{
    if (nib < 10)
        return nib + '0';
    else
        return nib + 'A' - 10;
}

void byte_to_hex(uint8_t byte, char *buf)
{
    buf[0] = nibble_to_hex(byte >> 4);
    buf[1] = nibble_to_hex(byte & 0x0F);
}

void uint16_to_hex(uint16_t val, char *buf)
{
    byte_to_hex(val >> 8, buf);
    byte_to_hex(val & 0xFF, buf + 2);
}

void uint32_to_hex(uint32_t val, char *buf)
{
    byte_to_hex(val >> 24, buf);
    byte_to_hex((val >> 16) & 0xFF, buf + 2);
    byte_to_hex((val >> 8) & 0xFF, buf + 4);
    byte_to_hex((val) & 0xFF, buf + 6);
}

uint16_t hex_to_uint16(char *hex)
{
    return (uint16_t)hex_to_byte(hex) << 8
         | (uint16_t)hex_to_byte(hex + 2);
}

uint32_t hex_to_uint32(char *hex)
{
    return (uint32_t)hex_to_byte(hex) << 24
         | (uint32_t)hex_to_byte(hex + 2) << 16
         | (uint32_t)hex_to_byte(hex + 4) << 8
         | (uint32_t)hex_to_byte(hex + 6);
}

