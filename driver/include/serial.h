#ifndef INCLUDE_SERIAL_H
#define INCLUDE_SERIAL_H

#include <stdint.h>

struct serial {
    uint8_t use_2x :1;
    uint16_t baud_register;
};

void serial_init(struct serial *);

static inline uint8_t serial_read(struct serial *serial)
{
    while (!(UCSR0A & _BV(RXC0)))
        ;

    return UDR0;
}

static inline void serial_write(struct serial *serial, uint8_t data)
{
    while (!(UCSR0A & _BV(UDRE0)))
        ;

    UDR0 = data;
}

#endif
