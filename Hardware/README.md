# Hardware

## Overview

Eagle Files, BOM, Status, and how to Test.

![Schematic](https://raw.githubusercontent.com/epccs/RPUadpt/master/Hardware/14226,Schematic.png "RPUadpt Schematic")

# Notes

```
Full Duplex RS-422 multi-drop RX and TX pairs.
Half Duplx RS-485 out of band management (DTR) pair.
Fail Safe state (high) when differential line is undriven.
Resting state of differential pair (RX, TX and DTR) is undriven.
Fits RPUno, Irrigate7, and PulseDAQ boards.
May also fit other Arduino compatible boards.
Power is taken from +5V of the MCU node board.
HOST may connect to the FTDI Friend pinout (3V3 logic).
I2C Interface between BUS manager and MCU node.  
BUS manager MCU type: ATmega328p
BUS manager MCU clock: 8MHz internal
BUS manager MCU Voltage: 3.3V (e.g. IOREF is 5V)
```

The manager can (with proper programming, and perhaps out of band instructions) lock the MCU node (and/or the HOST) from using the RX or TX lines.   