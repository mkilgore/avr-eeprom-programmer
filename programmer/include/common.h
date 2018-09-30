#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdint.h>

#ifndef VER_MAJOR
# define VER_MAJOR -1
#endif

#ifndef VER_MINOR
# define VER_MINOR -1
#endif

#ifndef VER_PATCH
# define VER_PATCH -1
#endif

/* Inspired via the Linux-kernel macro 'container_of' */
#define container_of(ptr, type, member) \
    ((type *) ((char*)(ptr) - offsetof(type, member)))

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))

#define QQ(x) #x
#define Q(x) QQ(x)

#define TP2(x, y) x ## y
#define TP(x, y) TP2(x, y)

static inline uint8_t is_hex_char(char c)
{
    switch (c) {
    case '0' ... '9':
    case 'A' ... 'F':
    case 'a' ... 'f':
        return 1;

    default:
        return 0;
    }
}

uint8_t hex_to_byte(char *);
void byte_to_hex(uint8_t byte, char *buf);
void uint16_to_hex(uint16_t val, char *buf);
void uint32_to_hex(uint32_t val, char *buf);

uint16_t hex_to_uint16(char *);
uint32_t hex_to_uint32(char *);

#endif
