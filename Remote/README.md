# Remote

This bus manager firmware is for an RPUadpt board, it will watch for a byte matching the local RPU_ADDRESS on the DTR pair and when seen reset the local MCU board (e.g. an RPUadpt on Irrigate7) which places it in bootloader mode. If a non-matching byte is seen the MCU is locked out from the RX and TX pair and no reset occures.

## Overview

In normal mode the RS-422 pairs have RX/TX driver and receiver enabled. The RS-485 (DTR) pair is connected to the bus manager UART and my accept bytes that are compared to the local RPU_ADDRESS or to a value for RPU_NORMAL_MODE (which connects the MCU to bus).

Durring lockout the RX pair receiver is disconnected from the local MCU RX input. While the TX pair driver is disconnected from the local MCU TX output.  

A byte on the DTR pair that matches the RPU_ADDRESS will cause a reset pulse to activate the bootloader. After bootloading an I2C command can be issued to send the RPU_NORMAL_MODE byte on the DTR pair.

When a byte on DTR does not match the RPU_ADDRESS it will cause the lockout condition that will last for a duration determined by the LOCKOUT_DELAY or when a RPU_NORMAL_MODE byte is seen on the DTR pair.

For how I setup my Makefile toolchain <http://epccs.org/indexes/Document/DvlpNotes/LinuxBoxCrossCompiler.html>.

With a ISP tool connection (set the ISP_PORT in Makefile) run 'make isp' and it should compile and then flash the MCU.

```
rsutherland@conversion:~/Samba/RPUadpt/Remote$ make isp
avr-gcc -Os -g -std=gnu99 -Wall -ffunction-sections -fdata-sections  -DF_CPU=8000000UL   -DBAUD=19200UL -I.  -mmcu=atmega328p -c -o main.o main.c
avr-gcc -Os -g -std=gnu99 -Wall -ffunction-sections -fdata-sections  -DF_CPU=8000000UL   -DBAUD=19200UL -I.  -mmcu=atmega328p -c -o ../lib/uart.o ../lib/uart.c
avr-gcc -Os -g -std=gnu99 -Wall -ffunction-sections -fdata-sections  -DF_CPU=8000000UL   -DBAUD=19200UL -I.  -mmcu=atmega328p -c -o ../lib/timers.o ../lib/timers.c
avr-gcc -Os -g -std=gnu99 -Wall -ffunction-sections -fdata-sections  -DF_CPU=8000000UL   -DBAUD=19200UL -I.  -mmcu=atmega328p -c -o ../lib/twi.o ../lib/twi.c
avr-gcc -Wl,-Map,Remote.map  -Wl,--gc-sections  -mmcu=atmega328p main.o ../lib/uart.o ../lib/timers.o ../lib/twi.o -o Remote.elf
avr-size -C --mcu=atmega328p Remote.elf
AVR Memory Usage
----------------
Device: atmega328p

Program:    3684 bytes (11.2% Full)
(.text + .data + .bootloader)

Data:        298 bytes (14.6% Full)
(.data + .bss + .noinit)


rm -f Remote.o main.o ../lib/uart.o ../lib/timers.o ../lib/twi.o
avr-objcopy -j .text -j .data -O ihex Remote.elf Remote.hex
rm -f Remote.elf
avrdude -v -p atmega328p -c stk500v1 -P /dev/ttyACM0 -b 19200 -e -U flash:w:Remote.hex -U lock:w:0x2f:m

avrdude: Version 6.2
         Copyright (c) 2000-2005 Brian Dean, http://www.bdmicro.com/
         Copyright (c) 2007-2014 Joerg Wunsch

         System wide configuration file is "/etc/avrdude.conf"
         User configuration file is "/home/rsutherland/.avrduderc"
         User configuration file does not exist or is not a regular file, skipping

         Using Port                    : /dev/ttyACM0
         Using Programmer              : stk500v1
         Overriding Baud Rate          : 19200
         AVR Part                      : ATmega328P
         Chip Erase delay              : 9000 us
         PAGEL                         : PD7
         BS2                           : PC2
         RESET disposition             : dedicated
         RETRY pulse                   : SCK
         serial program mode           : yes
         parallel program mode         : yes
         Timeout                       : 200
         StabDelay                     : 100
         CmdexeDelay                   : 25
         SyncLoops                     : 32
         ByteDelay                     : 0
         PollIndex                     : 3
         PollValue                     : 0x53
         Memory Detail                 :

                                  Block Poll               Page                       Polled
           Memory Type Mode Delay Size  Indx Paged  Size   Size #Pages MinW  MaxW   ReadBack
           ----------- ---- ----- ----- ---- ------ ------ ---- ------ ----- ----- ---------
           eeprom        65    20     4    0 no       1024    4      0  3600  3600 0xff 0xff
           flash         65     6   128    0 yes     32768  128    256  4500  4500 0xff 0xff
           lfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           hfuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           efuse          0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           lock           0     0     0    0 no          1    0      0  4500  4500 0x00 0x00
           calibration    0     0     0    0 no          1    0      0     0     0 0x00 0x00
           signature      0     0     0    0 no          3    0      0     0     0 0x00 0x00

         Programmer Type : STK500
         Description     : Atmel STK500 Version 1.x firmware
         Hardware Version: 2
         Firmware Version: 1.18
         Topcard         : Unknown
         Vtarget         : 0.0 V
         Varef           : 0.0 V
         Oscillator      : Off
         SCK period      : 0.1 us

avrdude: AVR device initialized and ready to accept instructions

Reading | ################################################## | 100% 0.02s

avrdude: Device signature = 0x1e950f (probably m328p)
avrdude: safemode: hfuse reads as DE
avrdude: safemode: efuse reads as FD
avrdude: erasing chip
avrdude: reading input file "Remote.hex"
avrdude: input file Remote.hex auto detected as Intel Hex
avrdude: writing flash (3684 bytes):

Writing | ################################################## | 100% 4.08s

avrdude: 3684 bytes of flash written
avrdude: verifying flash memory against Remote.hex:
avrdude: load data flash data from input file Remote.hex:
avrdude: input file Remote.hex auto detected as Intel Hex
avrdude: input file Remote.hex contains 3684 bytes
avrdude: reading on-chip flash data:

Reading | ################################################## | 100% 2.32s

avrdude: verifying ...
avrdude: 3684 bytes of flash verified
avrdude: reading input file "0x2f"
avrdude: writing lock (1 bytes):

Writing | ################################################## | 100% 0.03s

avrdude: 1 bytes of lock written
avrdude: verifying lock memory against 0x2f:
avrdude: load data lock data from input file 0x2f:
avrdude: input file 0x2f contains 1 bytes
avrdude: reading on-chip lock data:

Reading | ################################################## | 100% 0.01s

avrdude: verifying ...
avrdude: 1 bytes of lock verified

avrdude: safemode: hfuse reads as DE
avrdude: safemode: efuse reads as FD
avrdude: safemode: Fuses OK (E:FD, H:DE, L:E2)

avrdude done.  Thank you.
```

## Addressing

This firmware is at Address '1' on the RPU_BUS, (not 0x1 but the ASCII value 0x31 for the character).

When DTR toggles on an RPUftdi its Host2Remote firmware will send an address byte over the DTR pair. If the address sent matches the value in RPU_ADDRESS then the MCU RX/TX will be connected, for all other address the local MCU will be locked out. After a period of time in lockout or when a normal mode byte is seen, the MCU is (re)connected to RX/TX.

## I2C/TWI Slave

The I2C address is 0x29 (dec 41). It is organized as an array of read or write commands. Note: the sent data is used to size the reply, so add an extra byte after the command to size the reply.

0: reads this shields RPU_BUS addrss (default is '1', or from EE_PROM)
1: writes this shields RPU_BUS address to EE_PROM (not implemented)
2: read the address sent when DTR/RTS toggles 
3: write the address that will be sent when DTR/RTS toggles
4: reads TBD (not implemented)
5: wrtie normal RPU_BUS mode (send 0xFF on DTR pair) that will cancel lockout and connect all devices to the bus.
6: reads error status bits[0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match]
7: wrties (or clears) error status 


Connect to i2c-debug on an RPUno with an RPUftdi shield using picocom (or ilk). 

``` 
picocom -b 115200 /dev/ttyUSB0
``` 

Scan for the I2C slave address of the RPUftdi shield and set the byte that is sent when DTR/RTS toggles ('1' is 0x31 or 49).

``` 
/0/id?
{"id":{"name":"I2Cdebug","desc":"RPUno Board /w atmega328p and LT3652","avr-gcc":"4.9"}}
/0/scan?
{"scan":[{"addr":"0x29"}]}
/0/address 41
{"address":"0x29"}
/0/buffer 3,49
{"txBuffer":[{"data":"0x3"},{"data":"0x31"}]}
/0/read? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x31"}]}
``` 

exit picocom with C-a, C-x. 

Connect with picocom again. 

``` 
picocom -b 115200 /dev/ttyUSB0
``` 

This will toggle DTR on the RPUftdi shield which should send 0x31 on the DTR pair. The RPUftdi shield should blink slow to indicate lockout, while the RPUadpt blinks normal to indicate bootloader mode. The lockout should timeout after a delay that can be adjusted in firmware (Host2Remote or Remote) as needed.

Now connect to i2c-debug on an Irrigate7 with an RPUadpt shield. The local RPU address can be read.

``` 
/1/buffer 0,255
{"txBuffer":[{"data":"0x0"},{"data":"0xFF"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
``` 

Set normal mode to connect all devices to the RPU_BUS.

``` 
/1/buffer 5,255
{"txBuffer":[{"data":"0x5"},{"data":"0xFF"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x5"},{"data":"0xFF"}]}
``` 