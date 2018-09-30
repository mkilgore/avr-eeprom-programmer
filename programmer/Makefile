
TARGET := eepgmr
MAJOR := 0
MINOR := 9
PATCH := 0

CC := gcc

SRCS := $(wildcard ./src/*.c)
OBJS := $(SRCS:.c=.o)

CPPFLAGS := -I./include -I../include

CFLAGS += -O2 -g -std=gnu99 -Wall -fno-strict-aliasing \
		  -DVER_MAJOR=$(MAJOR) \
		  -DVER_MINOR=$(MINOR) \
		  -DVER_PATCH=$(PATCH)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

.PHONY: all clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

