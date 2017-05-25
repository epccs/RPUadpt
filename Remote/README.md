# Remote

This bus manager firmware is for an RPUadpt board, it will watch for a byte matching the local RPU_ADDRESS on the DTR pair and when seen reset the local MCU board placing it in bootloader mode. A non-matching byte will disconnect RS-422 from the RX and TX to the shield headers placing it in lockout mode.

## Overview

In normal mode, the RS-422 pairs RX and TX are connected to the shield RX and TX pins. While the RS-485 (DTR) pair is connected to the bus manager UART and used to set the system-wide bus state.

Durring lockout mode the RS-422 pairs RX and TX are disconnected from the shield RX and TX pins.

Bootload mode occures when a byte on the DTR pair matches the RPU_ADDRESS of the shield. It will cause a pulse on the shield reset pin to activate the bootloader on the board the shield is pluged into. After bootloading is done the shield will send the RPU_NORMAL_MODE byte on the DTR pair when the RPU_ADDRESS is read with an I2C command (otherwise the shield will timeout but not connect to the RS-422 bus).

lockout mode occures when a byte on the DTR pair does not match the RPU_ADDRESS of shield. It will cause the lockout condition and last for a duration determined by the LOCKOUT_DELAY or when a RPU_NORMAL_MODE byte is seen on the DTR pair.

When nRTS or nDTR are pulled active the bus manager will connect the HOST_TX and HOST_RX lines to the RX and TX pairs, and pull the nCTS and nDSR lines active to let the host know it is Ok to send. If the bus is in use the host remains disconnected from the bus. Note the Remote firmware sets a status bit at startup that prevents the host from connecting until it is cleared with an I2C command.

Arduino Mode (Work in progress, this is a list of ideas and may or may not be doable). It may be a sort of permeate bootload mode that the Arduino IDE can connect to. It would need to be enabled by a program on the board (an Uno) under an RPUftdi, which would set the mode with an I2C command. The command would send out a byte (on DTR pair) that enables a sticky bootload mode that is the point to point communication required for full duplex bootloaders and does not time out but does allow rerunning a proper bootload.


## Firmware Upload

Use an ICSP tool connected to the bus manager (set the ISP_PORT in Makefile) run 'make isp' and it should compile and then flash the bus manager.

```
rsutherland@conversion:~/Samba/RPUadpt/Remote$ make isp
...
avrdude done.  Thank you.
```

## Addressing

The Address '1' on the RPU_BUS is 0x31, (e.g. not 0x1 but the ASCII value for the character).

When HOST_nDTR (or HOST_nRTS) are pulled active from a host trying to connect to the RS-422 bus the local bus manager will set localhost_active and send the bootloader_address over the DTR pair. If an address received by way of the DTR pair matches the local RPU_ADDRESS the bus manager will enter bootloader mode (marked with bootloader_started), and connect the shield RX/TX to the RS-422 (see connect_bootload_mode() function), all other address are locked out. After a LOCKOUT_DELAY time or when a normal mode byte is seen on the DTR pair, the lockout ends and normal mode resumes. The node that has bootloader_started broadcast the return to normal mode byte on the DTR pair when that node has the RPU_ADDRESS read from the bus manager over I2C (otherwise it will time out and not connect the shield RX/TX to RS-422).


## Bus Manager Modes

In Normal Mode, the RPU bus manager connects the local MCU node to the RPU bus if it is RPU aware (e.g. ask for RPU address over I2C). Otherwise, it will not connect the local MCU's TX to the bus but does connect RX. The host will be connected unless it is foreign.

In bootload mode, the RPU bus manager connects the local MCU node to the RPU bus. Also, the host will be connected unless it is foreign. It is expected that all other nodes are in lockout mode. Note the BOOTLOADER_ACTIVE delay is less than the LOCKOUT_DELAY, but it needs to be in bootload mode long enough to allow finishing. A slow bootloader will require longer delays.

In lockout mode, if the host is foreign both the local MCU node and Host are disconnected from the bus, otherwise, the host remains connected.


## I2C/TWI Slave

The I2C address is 0x29 (dec 41). It is organized as an array of read or write commands. Note: the sent data is used to size the reply, so add an extra byte after the command to size the reply.

0. read the shields RPU_BUS addrss and activate normal mode (boadcast if localhost_active).
1. set the shields RPU_BUS address and write it (and an id) to eeprom
2. read the address sent when DTR/RTS toggles 
3. write the address that will be sent when DTR/RTS toggles
4. read RPUpi shutdown (the ICP1 pin has a weak pull up and a momentary switch)
5. set RPUpi shutdown (pull down ICP1 for SHUTDOWN_TIME in millis to cause host to hault)
6. reads status bits [0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match, 3:host lockout]
7. wrties (or clears) status 


Connect to i2c-debug on an RPUno with an RPUftdi shield using picocom (or ilk). 

``` 
picocom -b 115200 /dev/ttyUSB0
``` 


## Scan with i2c-debug 

Scan for the I2C slave address of the RPUftdi shield and address I2C.

``` 
/0/id?
{"id":{"name":"I2Cdebug","desc":"RPUno Board /w atmega328p and LT3652","avr-gcc":"4.9"}}
/0/scan?
{"scan":[{"addr":"0x29"}]}
/0/address 41
{"address":"0x29"}
```

## Read the RPUadpt shield address with i2c-debug

The local RPU address can be read.

``` 
/1/address 41
{"address":"0x29"}
/1/buffer 0,255
{"txBuffer":[{"data":"0x0"},{"data":"0xFF"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
``` 


## Set the bootload address with i2c-debug

Set the byte that is sent when DTR/RTS toggles ('2' is 0x32 or 50). Note RPUadpt has 3V3 logic inputs for a host.

```
/1/address 41
{"address":"0x29"}
/1/buffer 3,50
{"txBuffer":[{"data":"0x3"},{"data":"0x32"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x32"}]}
``` 

exit picocom with C-a, C-x. 

Connect with picocom again. 

``` 
picocom -b 115200 /dev/ttyUSB0
``` 

This will toggle DTR on the RPUadpt shield which should send 0x32 on the DTR pair. The RPUadpt shield should blink slow to indicate lockout, while the shield with address '2' blinks fast to indicate bootloader mode. The lockout should timeout after LOCKOUT_DELAY that can be adjusted in firmware.

Now connect to i2c-debug on an RPUno with the shield that has address '2'. The RPUno can read the address.

``` 
/2/address 41
{"address":"0x29"}
/2/buffer 0,255
{"txBuffer":[{"data":"0x0"},{"data":"0xFF"}]}
/2/read? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x32"}]}
``` 


## Set RPU_BUS address with i2c-debug

Using an RPUno and an RPUftdi shield, connect another RPUno with i2c-debug firmware to the RPUadpt shield that needs its address set. The default RPU_BUS address can be changed from '1' to any other value. 

``` 
/1/address 41
{"address":"0x29"}
/1/buffer 1,50
{"txBuffer":[{"data":"0x1"},{"data":"0x32"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x1"},{"data":"0x32"}]}
``` 

The example programs read the address durring setup, so they will need a reset.

```
/2/id?
{"id":{"name":"I2Cdebug","desc":"RPUno Board /w atmega328p and LT3652","avr-gcc":"4.9"}}
``` 

## Set RPUpi Shutdown

To hault the host send the I2C shutdown command 5 (first byte), with data 1 (second byte) which sets shutdown_started, clears shutdown_detected and pulls down the SHUTDOWN (ICP1) pin. The shutdown_started flag is also used to stop blinking of the LED_BUILTIN to reduce power usage noise so that the host power usage can be clearly seen.

``` 
/1/address 41
{"address":"0x29"}
/1/buffer 5,1
{"txBuffer":[{"data":"0x5"},{"data":"0x1"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x5"},{"data":"0x1"}]}
``` 

Above used an RPUftdi shield, connected to an RPUpi shield at address '1'. The shields were each mounted on an RPUno loaded with i2c-debug firmware.


## Read RPUpi Shutdown Detected

To check if host got a hault command or if the shutdown button got pressed send the I2C shutdown command 4 (first byte), with place holder data (second byte). This clears shutdown_detected flag that was used to keep the LED_BUILTIN from blinking.

``` 
/1/address 41
{"address":"0x29"}
/1/buffer 4,255
{"txBuffer":[{"data":"0x4"},{"data":"0xFF"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x1"}]}
/1/buffer 4,255
{"txBuffer":[{"data":"0x4"},{"data":"0xFF"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x0"}]}
``` 

Above used an RPUftdi shield, connected to an RPUpi shield at address '1'. The shields were each mounted on an RPUno loaded with i2c-debug firmware.

Second value in rxBuffer has shutdown_detected value (0 or 1). It is cleared when reading.


## Notes

If the program using a serial device (e.g. avrdude) gets sent a SIGINT (Ctrl+C) it may not leave the UART device driver in the proper state (e.g. the DTR/RTS may remain active). One way to clear this is to use modprobe to remove and reload the device driver.

``` 
# Serial restart (list device drivers)
lsmod | grep usbserial
# ftdi uses the ftdi_sio driver
# try to remove the driver
sudo modprobe -r ftdi_sio
# then load the driver again
sudo modprobe ftdi_sio
# the physcal UART chip will still have DTR/RTS active
# but picocom can open and release it now to clear the
# active lines.
picocom -b 115200 /dev/ttyUSB0
# C-a, C-x.
``` 
