#ifndef INCLUDE_SHIFT_H
#define INCLUDE_SHIFT_H

struct shift_register {
    volatile uint8_t *data_port;
    uint8_t data_pin;

    volatile uint8_t *clock_port;
    uint8_t clock_pin;

    volatile uint8_t *latch_port;
    uint8_t latch_pin;
};

void shift_register_write_bit(struct shift_register *reg, uint8_t bit);
void shift_register_write_byte(struct shift_register *reg, uint8_t value);
void shift_register_latch_output(struct shift_register *reg);

#endif
