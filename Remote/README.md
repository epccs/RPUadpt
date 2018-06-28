# Remote

This firmware is for the bus manager on an RPUadpt board, it will watch for a byte matching the local RPU_ADDRESS on the DTR pair and when seen reset the local MCU board placing it in bootloader mode. A non-matching byte will disconnect the RX and TX lines to the MCU board shield headers and thus be placing in lockout mode until a LOCKOUT_DELAY completes or a RPU_NORMAL_MODE byte is seen on the DTR pair.

## Overview

In normal mode, the serial pairs RX and TX are connected through tranceivers to the MCU board RX and TX pins. While the DTR pair is connected to the bus manager UART and used to set the system-wide bus state.

Durring lockout mode the serial pairs RX and TX are disconnected at the tranceivers from the MCU board RX and TX pins.

Bootload mode occures when a byte on the DTR pair matches the RPU_ADDRESS of the shield. It will cause a pulse on the shield reset pin to activate the bootloader on the board the shield is pluged into. After bootloading is done the shield will send the RPU_NORMAL_MODE byte on the DTR pair when the RPU_ADDRESS is read with an I2C command from the MCU board (otherwise the shield will timeout but not connect to the tranceivers to the MCU board RX and TX pins).

lockout mode occures when a byte on the DTR pair does not match the RPU_ADDRESS of shield. It will cause the lockout condition and last for a duration determined by the LOCKOUT_DELAY or when a RPU_NORMAL_MODE byte is seen on the DTR pair.

When nRTS or nDTR are pulled active the bus manager will connect the HOST_TX and HOST_RX lines to the RX and TX pairs, and pull the nCTS and nDSR lines active to let the host know it is Ok to send. If the bus is in use the host remains disconnected from the bus. Note the Remote firmware sets a status bit at startup that prevents the host from connecting until it is cleared with an I2C command.

Arduino Mode (Not Done, it is not yet even Work in progress, this is just a list of ideas). Perhaps a sort of permeate bootload mode that the Arduino IDE can connect to. It would need to be enabled by the host connected to a local bus managers I2C1 port with a new command. The command would send out a byte (on DTR pair) that enables a sticky_bootload mode (e.g. point to point communication).


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

The I2C0 address is 0x29 (dec 41) and I2C1 is 0x2A (dec 42). It is organized as an array of read or write commands. Note: the sent data is used to size the reply, so add a byte after the command for the manager to fill in with the reply.

0. read the shields RPU_BUS addrss and activate normal mode (boadcast if localhost_active).
1. set the shields RPU_BUS address and write it (and an id) to eeprom
2. read the address sent when DTR/RTS toggles 
3. write the address that will be sent when DTR/RTS toggles
4. read RPUpi shutdown (the ICP1 pin has a weak pull up and a momentary switch)
5. set RPUpi shutdown (pull down ICP1 for SHUTDOWN_TIME in millis to cause host to hault)
6. reads status bits [0:DTR readback timeout, 1:twi transmit fail, 2:DTR readback not match, 3:host lockout]
7. wrties (or clears) status 

The RPU has all commands available, but the HOST is limited to 0, 2, 3, 6, 7.

Connect to i2c-debug on an RPUno with an RPU shield using picocom (or ilk). 

``` 
picocom -b 38400 /dev/ttyUSB0
``` 


## RPU /w i2c-debug scan

Scan for the I2C0 slave address of shield and address.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/id?
{"id":{"name":"I2Cdebug^1","desc":"RPUno (14140^9) Board /w atmega328p","avr-gcc":"5.4.0"}}
/1/iscan?
{"scan":[{"addr":"0x29"}]}
```


## Raspberry Pi scan the shield address

I2C1 slave port is for host access. A Raspberry Pi is set up as follows.

```
sudo apt-get install i2c-tools python3-smbus
sudo usermod -a -G i2c rsutherland
# logout for the change to take
i2cdetect 1
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will probe file /dev/i2c-1.
I will probe address range 0x03-0x77.
Continue? [Y/n] Y
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- 2a -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```


## RPU /w i2c-debug read the shield address

The local RPU address can be read.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 0,255
{"txBuffer[2]":[{"data":"0x0"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x31"}]}
``` 


## Raspberry Pi read the shield address

The host can read the local RPU address on the I2C1 port.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 0, [255])
#read_i2c_block_data(I2C_ADDR, OFFSET, NUM_OF_BYTES)
# OFFSET is not implemented
print(bus.read_i2c_block_data(42, 0, 2))
[0, 49]
``` 


## RPU /w i2c-debug set RPU_BUS address

__Warning:__ this changes eeprom, flashing with ICSP does not clear eeprom due to fuse setting.

Using an RPUno and an RPUftdi shield, connect another RPUno with i2c-debug firmware to the RPUadpt shield that needs its address set. The default RPU_BUS address can be changed from '1' to any other value. 

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 1,50
{"txBuffer[2]":[{"data":"0x1"},{"data":"0x32"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x1"},{"data":"0x32"}]}
``` 

The RPU program normally reads the address during setup in which case it needs a reset to get the update. The reset can be done by setting its bootload address (bellow).

```
/2/id?
{"id":{"name":"I2Cdebug^1","desc":"RPUno (14140^9) Board /w atmega328p","avr-gcc":"5.4.0"}}
``` 


## RPU /w i2c-debug read the address sent when DTR/RTS toggles

I2C0 can be used by the RPU to Read the local bootload address that it will send when a host connects to it.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 2,255
{"txBuffer[2]":[{"data":"0x2"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x2"},{"data":"0x30"}]}
``` 

Address 0x30 is ascii '0' so the RPU at that location will be reset and connected to the serial bus when a host connects to the shield at address '1'. 


## Raspberry Pi read the address sent when DTR/RTS toggles

I2C1 can be used by the host to Read the local bootload address that it will send when a host connects to it.

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 2, [255])
#read_i2c_block_data(I2C_ADDR, OFFSET, NUM_OF_BYTES)
#OFFSET is not implemented
print(bus.read_i2c_block_data(42, 0, 2))
[2, 48]
``` 

Address 48 is 0x30 or ascii '0' so the RPU at that location will be reset and connected to the serial bus when the Raspberry Pi connects to the shield at this location (use command 1 to find the local address).


## RPU /w i2c-debug set the bootload address

__Note__: this valuse is not saved in eeprom so a power loss will set it back to '0'.

I2C0 can be used by the RPU to set the local bootload address that it will send when a host connects to it. When DTR/RTS toggles send ('2' is 0x32 is 50).

```
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 3,50
{"txBuffer[2]":[{"data":"0x3"},{"data":"0x32"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x32"}]}
``` 

exit picocom with C-a, C-x. 

Connect with picocom again. 

``` 
picocom -b 38400 /dev/ttyUSB0
``` 

This will toggle RTS on the RPUadpt shield and the manager will send 0x32 on the DTR pair. The RPUadpt shield should blink slow to indicate a lockout, while the shield with address '2' blinks fast to indicate bootloader mode. The lockout timeout LOCKOUT_DELAY can be adjusted in firmware.


## Raspberry Pi set the bootload address

__Note__: this valuse is not saved in eeprom so a power loss will set it back to '0'.

I2C1 can be used by the host to set the local bootload address that it will send when a host connects to it. When DTR/RTS toggles send ('2' is 0x32 is 50).

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 3, [50])
#read_i2c_block_data(I2C_ADDR, OFFSET, NUM_OF_BYTES)
#OFFSET is not implemented
print(bus.read_i2c_block_data(42,0, 2))
[3, 50]
``` 


## RPU /w i2c-debug read if HOST shutdown detected

To check if host got a manual hault command (e.g. if the shutdown button got pressed) send the I2C shutdown command 4 (first byte), with place holder data (second byte). This clears shutdown_detected flag that was used to keep the LED_BUILTIN from blinking.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 4,255
{"txBuffer[2]":[{"data":"0x4"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x1"}]}
/1/ibuff 4,255
{"txBuffer[2]":[{"data":"0x4"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x4"},{"data":"0x0"}]}
``` 

The above used a remote host and shield.

Second value in rxBuffer has shutdown_detected value 0x1, it is cleared after reading.


## RPU /w i2c-debug set HOST to shutdown

To hault the host send the I2C shutdown command 5 (first byte), with data 1 (second byte) which sets shutdown_started, clears shutdown_detected and pulls down the SHUTDOWN (ICP1) pin. The shutdown_started flag is also used to stop blinking of the LED_BUILTIN to reduce power usage noise so that the host power usage can be clearly seen.

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 5,1
{"txBuffer[2]":[{"data":"0x5"},{"data":"0x1"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x5"},{"data":"0x1"}]}
``` 

The above used a remote host and shield.


## RPU /w i2c-debug read status bits

I2C0 can be used by the RPU to read the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 6,255
{"txBuffer[2]":[{"data":"0x6"},{"data":"0xFF"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x6"},{"data":"0x8"}]}
``` 


## Raspberry Pi read status bits

I2C1 can be used by the host to read the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 6, [255])
#read_i2c_block_data(I2C_ADDR, OFFSET, NUM_OF_BYTES)
#OFFSET is not implemented
print(bus.read_i2c_block_data(42,0, 2))
[6, 8]
``` 

Bit 3 is set so the host can not connect until that has been cleared.


## RPU /w i2c-debug set status bits

I2C0 can be used by the RPU to set the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
picocom -b 38400 /dev/ttyUSB0
/1/iaddr 41
{"address":"0x29"}
/1/ibuff 7,8
{"txBuffer[2]":[{"data":"0x7"},{"data":"0x0"}]}
/1/iread? 2
{"rxBuffer":[{"data":"0x7"},{"data":"0x0"}]}
``` 

## Raspberry Pi sets status bits

I2C1 can be used by the host to read the local status bits.

0. DTR readback timeout
1. twi transmit fail 
2. DTR readback not match
3. host lockout

``` 
python3
import smbus
bus = smbus.SMBus(1)
#write_i2c_block_data(I2C_ADDR, I2C_COMMAND, DATA)
bus.write_i2c_block_data(42, 7, [0])
#read_i2c_block_data(I2C_ADDR, OFFSET, NUM_OF_BYTES)
#OFFSET is not implemented
print(bus.read_i2c_block_data(42,0, 2))
[7, 0]
exit()
picocom -b 38400 /dev/ttyAMA0
...
Terminal ready
/1/id?
# C-a, C-x.
``` 

The Raspberry Pi can bootload a target on the RPU serial bus.


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
picocom -b 38400 /dev/ttyUSB0
# C-a, C-x.
``` 
