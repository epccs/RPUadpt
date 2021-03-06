# BlinkLED is used to kick the tires
#
# to see the implicit rules which automagicly turn blinkLED.c into blinkLED.o run
# make -p
TARGET = blinkLED
LIBDIR = ../lib
OBJECTS = main.o \
	$(LIBDIR)/timers.o \
	$(LIBDIR)/twi1.o

## Chip and project-specific global definitions
MCU   =  atmega328pb
F_CPU = 12000000UL  
BAUD  =  250000UL
I2C1_ADDRESS = 0x2A
CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD)  -DI2C1_ADDRESS=$(I2C1_ADDRESS) -I. 

## Cross-compilation
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size

## Uno with Arduino as ISP sketch shows as ttyACMx
ISP_PORT = /dev/ttyACM0

## Compiler/linker options
CFLAGS = -Os -g -std=gnu99 -Wall
# CFLAGS += -funsigned-char -funsigned-bitfields 
# CFLAGS += -fpack-struct -fshort-enums 
CFLAGS += -ffunction-sections -fdata-sections 

## if atmega328pb is not in avr-gcc mainline use
TARGET_ARCH = -mmcu=$(MCU) -B $(LIBDIR)/Atmel.ATmega_DFP.1.2.209.atpack/gcc/dev/atmega328pb/ -I $(LIBDIR)/Atmel.ATmega_DFP.1.2.209.atpack/include/
## if atmega328pb is in avr-gcc mainline use
##TARGET_ARCH = -mmcu=$(MCU)


LDFLAGS = -Wl,-Map,$(TARGET).map 
LDFLAGS += -Wl,--gc-sections 

all: $(TARGET).hex $(TARGET).lst

$(TARGET): $(TARGET).hex

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

## stk500v1 is for Arduino as ISP (e.g. it is an example in the Arduino.cc IDE 1.6.7+)
## to reduce dupliction I only show the fuse setting in ../Bootloader

## lock the bootloader section, in case it is used some day
## if atmega328pb is not in /etc/avrdude.conf
isp: $(TARGET).hex
	avrdude -v -p $(MCU) -C +$(LIBDIR)/avrdude/328pb.conf -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U flash:w:$(TARGET).hex -U lock:w:0xef:m
## if atmega328pb is in /etc/avrdude.conf
#isp: $(TARGET).hex
#	avrdude -v -p $(MCU) -c stk500v1 -P $(ISP_PORT) -b 19200 -e -U flash:w:$(TARGET).hex -U lock:w:0x2f:m

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ -o $@
	$(SIZE) -C --mcu=$(MCU) $@
	rm -f $(TARGET).o $(OBJECTS)

clean: 
	rm -f $(TARGET).hex $(TARGET).map $(TARGET).elf $(TARGET).lst
 
%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@
