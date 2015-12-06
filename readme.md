# RPUadpt Software

From <http://epccs.org/hg/open/RPUadpt/>
Or <https://github.com/epccs/RPUadpt>

RPUadpt is a board I am working on that plugs into an Arduino Uno R3 pinout it has a few extra pins that match my RPUno, Irrigate7 and PulseDAQ boards. RPUadpt has an ATiny1634 that working with to act as a bus manager. 

With the Arduino ISP an ArduinoISP is used to program the ATiny1634 from an Uno at 5V through my ICSP (SPI) level converter board.

    <http://epccs.org/indexes/Board/ICSP/>

With Atmel Studio 7 a Dragon is used, however the ArduinoISP also works when setup with External Tools. 

    Command: C:\Program Files (x86)\Arduino\hardware\tools\avr\bin\avrdude.exe
    Arguments: -F -v -pattiny1634 -cstk500v1 -e -PCOM3 -b19200 -D -Uflash:w:"$(BinDir)\$(TargetName).hex":i -C"C:/Program Files (x86)/Arduino/hardware/tools/avr/etc/avrdude.conf"
    check_box for "Use Output window" selected 

![](https://raw.githubusercontent.com/epccs/RPUadpt/master/14226^1_ICSPwithDragon.jpg)

The core for this board is in epccs-avr, info can be found in the boards.txt file. 

    <http://epccs.org/hg/open/epccs-avr>
    <https://github.com/epccs/epccs-avr>


## NOTES

Github may not have the latest files.
    
I need a I2C slave library, but what I found did not support the 1634 so I 
may try modifying it. [TinyWireS][tinywires]
    
[tinywires]: https://github.com/epccs/TinyWireS/
    
