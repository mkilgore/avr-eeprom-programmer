
#include "common.h"

#include <avr/io.h>

#include "serial.h"

void serial_init(struct serial *serial)
{
    UBRR0L = serial->baud_register & 0x0F;
    UBRR0H = serial->baud_register >> 8;

    if (serial->use_2x)
        UCSR0A |= _BV(U2X0);
    else
        UCSR0A &= ~_BV(U2X0);

    /* Set format to 8 bits data, 1 stop bit */
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

    /* Turn on receiver and transmitter */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
}

