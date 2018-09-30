
#include "common.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#include "eeprom_tty.h"

static int setup_tty(int fd)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        perror("tcgetattr");
        return 1;
    }

    /* Setup various tty settings */
    tty.c_cflag |= (CLOCAL | CREAD); /* Turn off modem control */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8; /* 8 bits */
    tty.c_cflag &= ~PARENB; /* No parity */
    tty.c_cflag &= ~CSTOPB; /* One stop bit */
    tty.c_cflag &= ~CRTSCTS; /* No flow control */

    /* Setup non-canonical mode - this gives us characters as they are sent,
     * rather then any buffering */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* This ensures there are no delays in getting characters */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return 1;
    }

    return 0;
}

int eeprom_tty_set_baud(struct eeprom_tty *eeprom_tty, speed_t baud)
{
    struct termios tty;

    if (tcgetattr(eeprom_tty->fd, &tty) < 0) {
        perror("tcgetattr");
        return 1;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    if (tcsetattr(eeprom_tty->fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return 1;
    }

    return 0;
}

int eeprom_tty_open(struct eeprom_tty *tty, const char *tty_device)
{
    tty->fd = open(tty_device, O_RDWR | O_NOCTTY | O_SYNC);
    if (tty->fd < 0) {
        perror(tty_device);
        return 1;
    }

    if (setup_tty(tty->fd)) {
        close(tty->fd);
        return 1;
    }

    tty->in_file = fdopen(dup(tty->fd), "r");
    tty->out_file = fdopen(tty->fd, "w");

    return 0;
}

void eeprom_tty_close(struct eeprom_tty *tty)
{
    fclose(tty->in_file);
    fclose(tty->out_file);
}

void eeprom_tty_flush(struct eeprom_tty *tty)
{
    fflush(tty->out_file);
    tcdrain(tty->fd);
}

