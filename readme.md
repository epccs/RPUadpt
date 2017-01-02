# RPUadpt shield for RS-422 over CAT5 

![Status](https://raw.githubusercontent.com/epccs/RPUadpt/master/Hardware/status_icon.png "Status")

From <https://github.com/epccs/RPUadpt/>

## Overview

Shield used to connect a MCU node to a RPU_BUS, which is a multi-drop full duplex RS-422 (RX and TX pairs) with an out of band half duplex RS-485 (DTR pair).

![Schematic](https://raw.githubusercontent.com/epccs/RPUadpt/master/Hardware/14226,Schematic.png "RPUadpt Schematic")

![Bottom](https://raw.githubusercontent.com/epccs/RPUadpt/master/Hardware/14226,Bottom.png "RPUadpt Board Bottom")

![Top](https://raw.githubusercontent.com/epccs/RPUadpt/master/Hardware/14226,Top.png "RPUadpt Board Top")

[HackADay](https://hackaday.io/project/17719-rpuadpt).

[Forum](http://rpubus.org/bb/viewforum.php?f=7).

The core files are in the /lib folder while each example has its own Makefile.
    
## AVR toolchain

    * sudo apt-get install [gcc-avr](http://packages.ubuntu.com/search?keywords=gcc-avr)
    * sudo apt-get install [binutils-avr](http://packages.ubuntu.com/search?keywords=binutils-avr)
    * sudo apt-get install [gdb-avr](http://packages.ubuntu.com/search?keywords=gdb-avr)
    * sudo apt-get install [avr-libc](http://packages.ubuntu.com/search?keywords=avr-libc)
    * sudo apt-get install [avrdude](http://packages.ubuntu.com/search?keywords=avrdude)