# Software experiments

RPUadpt is a board I am working on that plugs into an Arduino Uno R3 pinout it does have a few extra pins that match my Irrigate7 and PulseDAQ boards. RPUadpt has an ATTINY1634 that I will most likely need to be programmed with Atmel Studio but who knows, perhaps the Arduino IDE can work also. 

First I just want to get things started with a simple blink program. That should ensure I've connected the ICSP pins correctly and get the fuse settings.

    Fuse Defaults not listed (except the clock setting)
    Low E2 :[CKDIV8:CKOUT:-:SUT:CKSEL3:CKSEL2:CKSEL1:CKSEL0] 1 1 1 0 0010
        Int. RC Osc. 8 MHz; Start-up time PWRDWN/RESET: 6 CK/14 CK + 64 ms; [CKSEL=0010 SUT=0]
    High D5: [RSTDISBL:DWEN:SPIEN:WDTON:EESAVE:BODLEVEL2:BODLEVEL1:BODLEVEL0] 1 1 0 1 0 101
        Preserve EEPROM memory through the Chip Erase cycle
        Brown-out detectijon level 2V7 (since I am running at 3V3)
    Extended F5 :[-:-:-:BODPD1:BODPD0:BODACT1:BODACT0:SELFPRGEN] 111 10 10 1
        Brown-Out Detection is on in both active and sleep modes (eats some power)
    Lock Bits 03 :[LB2:LB1] 11 No memory lock features enabled

    It is a little like an ATtiny167
    http://eleccelerator.com/fusecalc/fusecalc.php?chip=attiny167&LOW=E2&HIGH=D5&EXTENDED=F5&LOCKBIT=03