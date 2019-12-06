
#include "common.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "shift.h"
#include "am29.h"

struct shift_register address_low_16 = {
    .data_port = &PORTD,
    .data_pin = PORTD2,

    .latch_port = &PORTD,
    .latch_pin = PORTD3,

    .clock_port = &PORTD,
    .clock_pin = PORTD4,
};

#define CE_PIN PORTC3
#define OE_PIN PORTC4
#define WE_PIN PORTC5

static void am29_data_set_as_input(void)
{
    DDRB = 0;
    PORTB = 0xF0;
    _delay_ms(1);
}

static void am29_data_set_as_output(void)
{
    DDRB = 0xFF;
}

static void am29_setup_address(void)
{
    /* Turn on shift register outputs */
    DDRD |= 0b00011100;

    /* Turn on extra high 3 bits of address */
    DDRC |= 0x07;
}

static void am29_setup_control_lines(void)
{
    DDRC |= 0b00111000;
}

static void am29_teardown_gpio(void)
{
    DDRB = 0;
    DDRD &= ~0b00011100;
    DDRC &= ~0b00111111;
}

void am29_start(void)
{
    am29_setup_address();
    am29_setup_control_lines();
}

void am29_finish(void)
{
    am29_teardown_gpio();
}

static uint8_t am29_get_sector(uint32_t addr)
{
    return (addr & 0x70000) >> 16;
}

static uint16_t am29_get_addr(uint32_t addr)
{
    return addr & 0xFFFF;
}

/* Note: All of these 'enable' and 'disable' lines are active low */
static void am29_ce_enable(void)
{
    cbi(PORTC, CE_PIN);
}

static void am29_ce_disable(void)
{
    sbi(PORTC, CE_PIN);
}

static void am29_we_enable(void)
{
    cbi(PORTC, WE_PIN);
}

static void am29_we_disable(void)
{
    sbi(PORTC, WE_PIN);
}

static void am29_oe_enable(void)
{
    cbi(PORTC, OE_PIN);
}

static void am29_oe_disable(void)
{
    sbi(PORTC, OE_PIN);
}

static void am29_set_address(uint8_t sector, uint16_t addr)
{
    shift_register_write_byte(&address_low_16, addr >> 8);
    shift_register_write_byte(&address_low_16, addr & 0xFF);
    shift_register_latch_output(&address_low_16);

    PORTC &= ~0x07;
    PORTC |= sector & 0x07;
}

static uint8_t am29_read_byte_internal(uint8_t sector, uint16_t addr)
{
    uint8_t data;
    am29_set_address(sector, addr);
    am29_data_set_as_input();

    am29_ce_enable();
    am29_oe_enable();
    _delay_us(1);
    data = PINB;
    am29_oe_disable();
    am29_ce_disable();

    return data;
}

static void am29_write_byte_internal(uint8_t sector, uint16_t addr, uint8_t data)
{
    am29_set_address(sector, addr);
    am29_data_set_as_output();
    PORTB = data;

    am29_ce_enable();
    am29_we_enable();
    _delay_us(0.3);
    am29_we_disable();
    _delay_us(0.3);
    am29_ce_disable();
}

static void am29_program_byte(uint8_t sector, uint16_t addr, uint8_t data)
{
    am29_write_byte_internal(0, 0x555, 0xAA);
    am29_write_byte_internal(0, 0x2AA, 0x55);
    am29_write_byte_internal(0, 0x555, 0xA0);
    am29_write_byte_internal(sector, addr, data);
}

/* Returns zero on success, non-zero on failure */
static int verify_write(uint8_t sector, uint16_t addr, uint8_t data)
{
    uint8_t b1, b2;

    while (1) {
        b1 = am29_read_byte_internal(sector, addr);
        b2 = am29_read_byte_internal(sector, addr);

        /* (1 << 6) is the toggle bit, If it didn't toggle, then the operation is complete */
        if ((b1 & (1 << 6)) == (b2 & (1 << 6))) {
            return !(b1 == data);
        }

        /* Check if the time-limit has expired */
        if (b1 & (1 << 5)) {
            b1 = am29_read_byte_internal(sector, addr);
            b2 = am29_read_byte_internal(sector, addr);
            if ((b1 & (1 << 6)) == (b2 & (1 << 6)))
                return !(b1 == data);
            else
                return 1;
        }
    }
}

int am29_erase_chip(void)
{
    am29_write_byte_internal(0, 0x555, 0xAA);
    am29_write_byte_internal(0, 0x2AA, 0x55);
    am29_write_byte_internal(0, 0x555, 0x80);
    am29_write_byte_internal(0, 0x555, 0xAA);
    am29_write_byte_internal(0, 0x2AA, 0x55);
    am29_write_byte_internal(0, 0x555, 0x10);

    return verify_write(0, 0, 0xFF);
}

int am29_erase_sector(uint8_t sector)
{
    am29_write_byte_internal(0, 0x555, 0xAA);
    am29_write_byte_internal(0, 0x2AA, 0x55);
    am29_write_byte_internal(0, 0x555, 0x80);
    am29_write_byte_internal(0, 0x555, 0xAA);
    am29_write_byte_internal(0, 0x2AA, 0x55);
    am29_write_byte_internal(0, ((uint16_t)sector << 8) | 0xA, 0x30);

    return verify_write(sector, 0, 0xFF);
}

int am29_write_byte(uint32_t addr32, uint8_t data)
{
    uint8_t sector = am29_get_sector(addr32);
    uint16_t addr = am29_get_addr(addr32);

    int i;

    for (i = 0; i < 5; i++) {
        am29_program_byte(sector, addr, data);

        if (verify_write(sector, addr, data))
            return 1;
    }

    return 0;
}

uint8_t am29_read_byte(uint32_t addr)
{
    return am29_read_byte_internal(am29_get_sector(addr), am29_get_addr(addr));
}

