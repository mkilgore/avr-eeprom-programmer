#ifndef INCLUDE_EEPROM_TTY_H
#define INCLUDE_EEPROM_TTY_H

#include <termios.h>
#include <stdio.h>

struct eeprom_tty {
    int fd;

    FILE *in_file;
    FILE *out_file;
};

int eeprom_tty_open(struct eeprom_tty *, const char *tty_device);
int eeprom_tty_set_baud(struct eeprom_tty *eeprom_tty, speed_t baud);
void eeprom_tty_close(struct eeprom_tty *);

void eeprom_tty_flush(struct eeprom_tty *);

#endif
