# RPUadpt

From <https://github.com/epccs/RPUadpt/>

## Overview

Shield used to connect a microcontroler to a full duplex RS-422 (RX and TX pairs) and an out of band half duplex RS-485 (DTR pair) over CAT5. Its a multidrop bus between a host (e.g. Pi Zero on [RPUpi] or desktop with [RPUftdi]) and an MCU board (e.g. [RPUno]).

[HackADay](https://hackaday.io/project/17719-rpuadpt)

[Forum](http://rpubus.org/bb/viewforum.php?f=7)

[OSHpark ^4](https://oshpark.com/shared_projects/E8B1i7ss)

[RPUno]: https://github.com/epccs/RPUno
[RPUpi]: https://github.com/epccs/RPUpi
[RPUftdi]: https://github.com/epccs/RPUftdi

## Status

![Status](./Hardware/status_icon.png "Status")

At this time using this shield will require programming with an ICSP tool that is able to program a 3.3V ATmega328p target. 


## [Hardware](./Hardware)

Hardware files are in Eagle, there is also some testing, evaluation, and schooling notes for referance.


## Example

A multi-drop serial bus allows multiple microcontroller boards to be connected to a host serial port. The host computer crossover occurs on an [RPUftdi] or [RPUpi] shield. The host and microcontrollers control the transceivers differential driver automatically, which means no software [magic] is needed, though only one microcontroller should be allowed to talk. 

[magic]: https://github.com/pyserial/pyserial/blob/master/serial/rs485.py

![MultiDrop](./Hardware/Documents/MultiDrop.png "MultiDrop")

In the examples, for [RPUno] a command processor is used to accept interactive textual commands and operate the peripherals. The examples have a simple makefile that compiles the microcontroller firmware from the source. The host I used has Ubuntu (or Raspbian) with the AVR toolchain installed.

The RPUno examples have a makefile with a bootload rule (e.g. "make bootload") that uploads to the targets bootloader using avrdude.

When PySerial on the host opens the serial port it pulls the nDTR line low (it is active low) and that tells the [RPUftdi] manager running [Host2Remote] firmware to reset the bootload address. PySerial needs to wait for a few seconds while the bootloader timeout finishes just like with an Arduino Uno, and is the same reason (e.g. on RS232 the same bootloader timeout is needed).

[Host2Remote]: https://github.com/epccs/RPUftdi/tree/master/Host2Remote

When avrdude opens the serial port it pulls the nDTR line low and the manager broadcast the bootload address which places everything in lockout except the host and the microcontroller board that was addressed. The address is held in the manager on the RPUadpt shield which is running [Remote] firmware. Replacing the [RPUno] dos not change the address of the location, but replacing the RPUadpt or modify its address (by I2C) does.

[Remote]: ./Remote



## AVR toolchain

* sudo apt-get install [gcc-avr]
* sudo apt-get install [binutils-avr]
* sudo apt-get install [gdb-avr]
* sudo apt-get install [avr-libc]
* sudo apt-get install [avrdude]
    
[gcc-avr]: http://packages.ubuntu.com/search?keywords=gcc-avr
[binutils-avr]: http://packages.ubuntu.com/search?keywords=binutils-avr
[gdb-avr]: http://packages.ubuntu.com/search?keywords=gdb-avr
[avr-libc]: http://packages.ubuntu.com/search?keywords=avr-libc
[avrdude]: http://packages.ubuntu.com/search?keywords=avrdude