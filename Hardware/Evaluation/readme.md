# Description

This shows the setup and method used for evaluation of RPUadpt.

# Table of References


# Table Of Contents:

5. [^5 South Wall Enclosure](#3-remote-bootload)
4. [^3 Remote Bootload](#3-remote-bootload)
3. [^1 Mounts on Irrigate7](#1-mounts-on-irrigate7)
2. [^1 Mounts on Uno](#1-mounts-on-uno)
1. [^1 ICSP With Dragon](#1-icsp-with-dragon)


## ^5 South Wall Enclosure

![South Wall Enclosure](./14226^5_SWallEnclWithRPUnoAndK3.jpg "South Wall Enclosure")

It is mounted on an RPUno that controls some latching solenoids. 

I had had the previous version in the Enclosure but it had some hacks, this one is clean (though my wiring is a mess, that is not what I'm trying to show).


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


