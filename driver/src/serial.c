
#include "common.h"

#include <avr/io.h>
#include <stdio.h>

#include "serial.h"

static int put_char(char c, FILE *filp)
{
    serial_write(NULL, c);
    return 0;
}

void serial_init(struct serial *serial)
{
    UBRR0L = serial->baud_register & 0xFF;
    UBRR0H = serial->baud_register >> 8;

    if (serial->use_2x)
        UCSR0A |= _BV(U2X0);
    else
        UCSR0A &= ~_BV(U2X0);

    /* Set format to 8 bits data, 1 stop bit */
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

    /* Turn on receiver and transmitter */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);

    fdevopen(put_char, NULL);
}

