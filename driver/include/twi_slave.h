#ifndef INCLUDE_TWI_SLAVE_H
#define INCLUDE_TWI_SLAVE_H

#include <inttypes.h>
#include <avr/interrupt.h>

enum twi_state {
    TWI_STATE_READ_WRITE,
    TWI_STATE_RECV_REG,
};

struct twi_handler {
    void (*recv) (struct twi_handler *, uint8_t data);
    uint8_t (*request) (struct twi_handler *);

    uint8_t cur_reg;
    uint8_t address;
    enum twi_state state;
};

void twi_init(struct twi_handler *);

#endif
