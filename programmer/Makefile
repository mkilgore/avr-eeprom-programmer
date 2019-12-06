
TARGET := eepgmr
MAJOR := 0
MINOR := 9
PATCH := 0

CC := gcc

SRCS := $(wildcard ./src/*.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
DEPS := $(foreach dep,$(DEPS),$(dir $(dep)).$(notdir $(dep)))

CPPFLAGS := -I./include -I../include

CFLAGS += -O2 -g -std=gnu99 -Wall -fno-strict-aliasing \
		  -DVER_MAJOR=$(MAJOR) \
		  -DVER_MINOR=$(MINOR) \
		  -DVER_PATCH=$(PATCH)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.%.d: %.c
	$(CC) -MM -MP -MF $@ $(CPPFLAGS) $(CFLAGS) $< -MT $*.o -MT $@

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

.PHONY: all clean

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -f $(DEPS)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

