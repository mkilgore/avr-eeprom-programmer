#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#define USE_PRINTF

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <inttypes.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

static inline uint8_t is_hex_char(uint8_t c)
{
    switch (c) {
    case '0' ... '9':
    case 'A' ... 'F':
    case 'a' ... 'f':
        return 1;

    default:
        return 0;
    }
}

uint8_t hex_to_byte(uint8_t *);
void byte_to_hex(uint8_t byte, uint8_t *buf);
void uint16_to_hex(uint16_t val, uint8_t *buf);
void uint32_to_hex(uint32_t val, uint8_t *buf);

uint16_t hex_to_uint16(uint8_t *);
uint32_t hex_to_uint32(uint8_t *);

#endif
