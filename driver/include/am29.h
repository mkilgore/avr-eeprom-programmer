#ifndef INLCUDE_AM29_H
#define INLCUDE_AM29_H

#include <stdint.h>

/* Initializes all of the GPIO ports for reading/writing to the AM29
 * am29_finish() puts them all back into the High-Z state (Input) */
void am29_start(void);
void am29_finish(void);

/* These functions return zero on success, and non-zero on failure */
int am29_erase_chip(void);
int am29_erase_sector(uint8_t sector);
int am29_write_byte(uint32_t addr, uint8_t data);

#define AM29_SECTOR_SIZE (1024 * 64)
#define AM28_SECTOR_COUNT 8
#define AM29_SECTOR_MASK  0x7

uint8_t am29_read_byte(uint32_t addr);

#endif
