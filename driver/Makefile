
TARGET := eeprog

MCU := atmega328p
F_CPU := 8000000UL
BAUD := 9600UL
AVRDUDE_MCU := atmega328p

PROGRAMMER_TYPE := usbasp
PROGRAMMER_ARGS :=

PREFIX := avr-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
AVRSIZE := avr-size
AVRDUDE := avrdude

SRCS := $(wildcard ./src/*.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
DEPS := $(foreach dep,$(DEPS),$(dir $(dep)).$(notdir $(dep)))

CPPFLAGS := -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I./include -I../include

CFLAGS += -Os -g -std=gnu99 -Wall
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -fwrapv

CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS := -wl,-Map,$(TARGET).map
LDFLAGS := -Wl,--gc-sections

TARGET_ARCH := -mmcu=$(MCU)

all: $(TARGET).hex $(TARGET).eeprom

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

.%.d: %.c
	$(CC) -MM -MP -MF $@ $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) $< -MT $*.o -MT $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.eeprom: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%.lst: %.elf
	$(OBJDUMP) -S $< > $@

.PHONY: all eeprom clean flash flash_eeprom fuses show_fuses

clean:
	rm -f $(OBJS)
	rm -f $(TARGET).elf $(TARGET).hex $(TARGET).eeprom
	rm -f $(DEPS)

flash: $(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(AVRDUDE_MCU) $(PROGRAMMER_ARGS) -U flash:w:$<

flash_eeprom: $(TARGET).eeprom
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(AVRDUDE_MCU) $(PROGRAMMER_ARGS) -U eeprom:w:$<

LFUSE := 0xE2
HFUSE := 0xD9
EFUSE := 0xFF

FUSE_STRING := -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

fuses:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(AVRDUDE_MCU) \
		$(PROGRAMMER_ARGS) $(FUSE_STRING)

show_fuses:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(AVRDUDE_MCU) $(PROGRAMMER_ARGS) -nv

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

