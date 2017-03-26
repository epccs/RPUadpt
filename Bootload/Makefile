# Makefile 
# Used to set fues, the RPUadpt which does not have a bootloader at this time 
TARGET = Bootload
LIBDIR = ../lib
OBJECTS = 

## I use an ArduinoISP example sketch form Arduino.cc IDE 1.6.7+ loaded in an Arduino.org Uno (it is clear as mud right) that shows as /dev/ttyACMx
##/dev/ttyACM0

all:

$(TARGET):

# max sketch size: 30720 Diecimila (2k byt,bootsz 1k wrd), 32256 Uno (512 byt,bootsz 256 wrd), 32768 (nil bootsz)
# AVR instructions are essentially words, e.g. multiples of 16bits (except reading data from program memory, LPM)

# 12MHz Crystal Osc, slowly rising power, 16K CK startup time, 14CK +65ms reset delay.
# BOOTRST programmed so boot reset vector is enabled (in case a bootloader is added)
# EESAVE programmed to preserve EEPROM thru chip erase 
# 2v7 B.O.D. Enabled       note extended fuse value 0x05 is same as 0xFD since bits 3 thru 7 are hardware locked

## http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p&LOW=FF&HIGH=D6&EXTENDED=05&LOCKBIT=3F
## stk500v1 is for Arduino as ISP (e.g. it is an example in the Arduino.cc IDE 1.6.7+)
fuse: 
	avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U lock:w:0x3F:m -U lfuse:w:0xFF:m -U hfuse:w:0xD6:m -U efuse:w:0xFD:m

read: 
	avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -U lock:r:0x3F:m -U lfuse:r:0xFF:m -U hfuse:r:0xD6:m -U efuse:r:0xFD:m

# note this is the old setting I used befor switching to the crystal.
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

 
