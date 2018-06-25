# Description

This shows the setup and method used for evaluation of RPUadpt.

# Table of References


# Table Of Contents:

1. [^6 I2C1 Checked With i2c-debug](#6-i2c1-checked-with-i2c-debug)
1. [^5 Remote Reset](#5-remote-reset)
1. [^5 Bus Termination](#5-bus-termination)
1. [^5 South Wall Enclosure](#5-south-wall-enclosure)
1. [^3 Remote Bootload](#3-remote-bootload)
1. [^1 Mounts on Irrigate7](#1-mounts-on-irrigate7)
1. [^1 Mounts on Uno](#1-mounts-on-uno)
1. [^1 ICSP With Dragon](#1-icsp-with-dragon)


## ^6 I2C1 Checked With i2c-debug

[BlinkLED] allows the I2C1 port (e.g. the secnond I2C port on ATmega328pb) to interface with a host SBC.

[BlinkLED]: https://github.com/epccs/RPUadpt/tree/master/BlinkLED

The [i2c-debug] firmware can be used to test the I2C1 port, its address is at 0x2A. The Uno clone running [i2c-debug] has another I2C interface at 0x29. 

[i2c-debug]: https://github.com/epccs/RPUno/tree/master/i2c-debug

```
picocom -b 38400 /dev/ttyUSB0
...
Terminal ready
/0/iscan?
{"scan":[{"addr":"0x29"},{"addr":"0x2A"}]}
/0/iaddr 42
{"address":"0x2A"}
/0/ibuff 0,3
{"txBuffer[2]":[{"data":"0x0"},{"data":"0x3"}]}
/0/iwrite
{"returnCode":"success"}
```

blinking has stopped

```
/0/ibuff 0
{"txBuffer[2]":[{"data":"0x0"}]}
/0/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0xFC"}]}
```

blinking has resumed

Note: I incuded the command byte befor reading the data because I think that is what SMBus does.

![^6 I2C1_Checked_With_i2c-debug](./RPUadpt^6_with_fwBlinkLED_UnoClone_with_i2c-debug.jpg "^6 I2C1 Checked With i2c-debug")


## ^5 Remote Reset

I used an RPUftdi^4 to do a remote reset of an RPUno^6 with an RPUadpt^5 shield. The RPUno was also wired to a K3^2 board. A quick video shows picocom used to do the remote reset. 

[^5 Remote Reset Video](http://rpubus.org/Video/RPUno%5E6_RPUadpt%5E5_RPUftdi%5E4_K3%5E2_RemoteReset.mp4 "^5 Remote Reset Video")

![^5 Remote Reset](./RPUadpt^5_RPUno^6_K3^2_RPUftdi^4_RemoteReset.jpg "^5 Remote Reset")

After the reset, the bootloader runs for a few seconds and then passes control to the [Solenoid] firmware which reads an address from the RPUadpt shield over I2C and cycles through each latching coil to place them in a known state. Reading the address from RPUadpt also lets the bus manager on that board broadcast a byte on the RS-485 bus management pair (DTR) that ends the lockout placed on other devices to allow a point to point bootload connection (i.e. a return to normal point to multipoint mode).

[Solenoid]: https://github.com/epccs/RPUno/tree/master/Solenoid


## ^5 Bus Termination

![Bus Termination](./14226^5_RPU_busTermination.jpg "Bus Termination")

The RX, TX, and DTR pair need a 100 Ohm termination on each end of the CAT5 daisy chain. There are places on the board to solder the resistors, they are on the RPUftdi, but for an RPUadpt an RJ45 plug allows adding to the daisy chain without soldering. I'm going to supply this for now since it is how I'm using the RPUadpt board.


## ^5 South Wall Enclosure

![South Wall Enclosure](./14226^5_SWallEnclWithRPUnoAndK3.jpg "South Wall Enclosure")

It is mounted on an RPUno that controls some latching solenoids. 

I had had the previous version in the Enclosure but it had some hacks, this one is clean (though my wiring is a mess, that is not what I'm trying to show).

DTR pair is running at 250k bits/sec, and the 12MHz crystal is enabled.


## ^3 Remote Bootload

The proof of concept (e.g. Multi-Drop Remote Bootload) used [Host2Remote] firmware on a RPUftdi^3 and [i2c-debug] on its bare metal MCU board (an OSEPP Uno R3 from fry's electronics). The remote device has [Remote] firmware on a RPUadpt^3 board mounted on an Irrigate7^1 (which is an ATmega1284p with xboot). The firmware bootloaded onto the Irrigate7^1 is [BlinkLED].

[Remote Bootload Video](http://rpubus.org/Video/14145%5E3_RPU_RemoteBootload.mp4 "Remote Bootload Video")

![Remote Bootload](./14226^3_RemoteBootload.jpg "Remote Bootload")

[Host2Remote]: https://github.com/epccs/RPUftdi/tree/master/Host2Remote
[Remote]: https://github.com/epccs/RPUadpt/tree/master/Remote
[BlinkLED]: https://github.com/epccs/Irrigate7/tree/master/BlinkLED
[i2c-debug]: https://github.com/epccs/RPUno/tree/master/i2c-debug


## ^1 Mounts on Irrigate7

Check that it fits on Irrigate7 and pin functions match.

![RPUadpt On Irrigate7](./14226^1_OnIrrigate7.jpg "RPUadpt On Irrigate7")


## ^1 Mounts on Uno

Check that it fits on Uno and pin functions match.

![RPUadpt On Uno](./14226^1_OnUno.jpg "RPUadpt On Uno")

The extra pins do not interfere with Uno.

![RPUadpt On Uno Show Extra Pins](./14226^1_OnUnoShowingExtraPins.jpg "RPUadpt On Uno Show Extra Pins")

The USB and power do not interfere with Uno (note USB has been removed after this version)

![RPUadpt On Uno USB View](./14226^1_OnUnoUSBView.jpg "RPUadpt On Uno USB View")


## ^1 ICSP With Dragon

A little blink program done in Atmel Studio 7 to see if the ATtiny1634 ICSP was working. 

![RPUadpt ICSP With Dragon](./14226^1_ICSPwithDragon.jpg "RPUadpt ICSP With Dragon")

Note the ATtiny1634 was changed to ATmega328p after this version, see [Schooling](../Schooling).


