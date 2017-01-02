# Remote

This bus manager firmware is for an RPUadpt board, it will watch for a byte matching the local RPU_ADDRESS on the DTR pair and when seen reset the local MCU board (e.g. an RPUadpt on Irrigate7) which places it in bootloader mode. If a non-matching byte is seen the MCU is locked out from the RX and TX pair and no reset occures.

## Overview

In normal mode the RS-422 pairs have RX/TX driver and receiver enabled. The RS-485 (DTR) pair is connected to the bus manager UART and my accept bytes that are compared to the local RPU_ADDRESS or to a value for RPU_NORMAL_MODE (which connects the MCU to bus).

Durring lockout the RX pair receiver is disconnected from the local MCU RX input. While the TX pair driver is disconnected from the local MCU TX output.  

A byte on the DTR pair that matches the RPU_ADDRESS will cause a reset pulse to activate the bootloader. After bootloading an I2C command can be issued to send the RPU_NORMAL_MODE byte on the DTR pair.

When a byte on DTR does not match the RPU_ADDRESS it will cause the lockout condition that will last for a duration determined by the LOCKOUT_DELAY or when a RPU_NORMAL_MODE byte is seen on the DTR pair.

## Firmware Upload

With an ISP tool connected to the bus manager (set the ISP_PORT in Makefile) run 'make isp' and it should compile and then flash the bus manager.

```
rsutherland@conversion:~/Samba/RPUadpt/Remote$ make isp
...
avrdude done.  Thank you.
```

## Addressing

The Address '1' on the RPU_BUS is 0x31, (e.g. not 0x1 but the ASCII value for the character).

When DTR (or RTS) toggles from a host connected to the RPU bus the receiving bus manager will set localhost_active and send the bootloader_address over the DTR pair. If an address received by way of the DTR pair matches the local RPU_ADDRESS the bus manager will enter bootloader mode (marked with bootloader_started), and connect the node MCU to the RPU_BUS (see connect_bootload_mode() function), all other address should be locked out. After a LOCKOUT_DELAY time or when a normal mode byte is seen on the DTR pair, the lockout ends and normal mode resumes. The node that has bootloader_started broadcast the return to normal mode byte on the DTR pair when the node reads the RPU_ADDRESS from the bus manager over I2C (otherwise it will time out).


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
4. reads TBD (not implemented)
5. write TBD (not implemented)
6. reads error status bits[0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match]
7. wrties (or clears) error status 


Connect to i2c-debug on an RPUno with an RPUftdi shield using picocom (or ilk). 

``` 
picocom -b 115200 /dev/ttyUSB0
``` 


## Scan with i2c-debug 

Scan for the I2C slave address of the RPUftdi shield and set the byte that is sent when DTR/RTS toggles ('1' is 0x31 or 49).

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
