# Makefile 
# Used to set fues, the RPUadpt which does not have a bootloader at this time 
TARGET = RPUadpt_fuse
LIBDIR = ../lib
OBJECTS = 

#  http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=FF&HIGH=D6&EXTENDED=05&LOCKBIT=3F
LOW_FUSE = 0xFF
HIGH_FUSE = 0xD6
# the efuse uses the lower three bits, so some tools read it as 0x05.
EXTENDED_FUSE = 0xFD
# lockbit uses the lower five bits
# on avrdude -v <= 6.2 the -c stk500v1 tool returns "0" for unused bits 
#UNLOCK_FUSE = 0x3F
#LOCK_FUSE = 0x2F
# on avrdude -v >= 6.3 the -c stk500v1 tool works like the other tools
UNLOCK_FUSE = 0xFF
LOCK_FUSE = 0xEF

all:

$(TARGET):

# max sketch size: 30720 Diecimila (2k byt,bootsz 1k wrd), 32256 Uno (512 byt,bootsz 256 wrd), 32768 (nil bootsz)
# AVR instructions are essentially words, e.g. multiples of 16bits (except reading data from program memory, LPM)

# 12MHz Crystal Osc, slowly rising power, 16K CK startup time, 14CK +65ms reset delay.
# BOOTRST programmed so boot reset vector is enabled (in case a bootloader is added)
# EESAVE programmed to preserve EEPROM thru chip erase 
# 2v7 B.O.D. Enabled       note extended fuse value 0x05 is same as 0xFD since bits 3 thru 7 are hardware locked

## http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=FF&HIGH=D6&EXTENDED=05&LOCKBIT=3F
## stk500v1 is for Arduino as ISP (e.g. an example in the Arduino.cc IDE 1.6.7+) that mounts at /dev/ttyACM0
## dragon_isp is for the AVRdragon tool
## atmega328pb is not in mainline,  when it is in mainline remove "-C +../lib/avrdude/328pb.conf"
fuse: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
read: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c stk500v1 -P /dev/ttyACM0 -b 19200 -U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
dfuse: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c dragon_isp -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m
dread: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c dragon_isp -U lock:r:-:h -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

## set fuse like they are from factory, i.e. 8MHz internal, B.O.D. disabled.
oem: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:0x62:m -U hfuse:w:0xD9:m -U efuse:w:0xF7:m
doem: 
	avrdude -v -C +../lib/avrdude/328pb.conf -p atmega328pb -c dragon_isp -e -U lock:w:$(UNLOCK_FUSE):m -U lfuse:w:0x62:m -U hfuse:w:0xD9:m -U efuse:w:0xF7:m

# note this is the old setting I used befor using the crystal.
# 8MHz internal
# BOOTRST programmed so boot reset vector is enabled (in case a bootloader is added)
# 2v7 B.O.D. Enabled       note extended fuse value 0x05 is same as 0xFD since bits 3 thru 7 are hardware locked

## http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=E2&HIGH=DE&EXTENDED=05&LOCKBIT=3F
## ArduinoISP acts as a stk500v1 programing device 
fuse_old: 
	avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:0x3f:m -U lfuse:w:0xE2:m -U hfuse:w:0xDE:m -U efuse:w:0xFD:m

## Use “-” as the file name, to write to stdout
read_old: 
	avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

 
