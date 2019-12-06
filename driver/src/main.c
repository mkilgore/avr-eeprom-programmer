
#include "common.h"
#include "shift.h"
#include "msg.h"

#include <stdio.h>

#include <avr/interrupt.h>

#include "am29.h"

static struct serial serial = {
    .use_2x = 1,
    .baud_register = 25, /* This uses 250K baud */
};

int main(void)
{
    serial_init(&serial);

    msg_loop(&serial);
}

