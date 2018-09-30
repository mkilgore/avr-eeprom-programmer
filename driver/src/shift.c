
#include "common.h"

#include <util/delay.h>

#include "shift.h"

void shift_register_write_bit(struct shift_register *reg, uint8_t bit)
{
    if (bit)
        sbi(*reg->data_port, reg->data_pin);
    else
        cbi(*reg->data_port, reg->data_pin);

    sbi(*reg->clock_port, reg->clock_pin);
    cbi(*reg->clock_port, reg->clock_pin);
}

void shift_register_write_byte(struct shift_register *reg, uint8_t value)
{
    int i;
    for (i = 0; i < 8; i++) {
        shift_register_write_bit(reg, value & 0x80);
        value <<= 1;
    }
}

void shift_register_latch_output(struct shift_register *reg)
{
    sbi(*reg->latch_port, reg->latch_pin);
    cbi(*reg->latch_port, reg->latch_pin);
}

