# Description

This is a list of Test preformed on each RPUadpt after assembly.

# Table of References


# Table Of Contents:

1. [Basics](#basics)
2. [Assembly check](#assembly-check)
3. [IC Solder Test](#ic-solder-test)
4. [Bias +5V and Check LDO Regulator](#bias-5v-and-check-ldo-regulator)
5. [Set MCU Fuse](#set-mcu-fuse)
6. [Load CheckDTR Firmware](#load-checkdtr-firmware)
7. [Check Differential Bias](#check-differential-bias)
8. [Differential Loopback with TX Driver](#differential-loopback-with-tx-driver)
9. [Differential Loopback with RX Driver](#differential-loopback-with-rx-driver)
10. [Load i2c-debug into an MCU board](#load-i2c-debug-into-an-mcu-board)
11. [I2C Slave test](#i2c-slave-test)



## Basics

These tests are for an assembled RPUadpt board 14226^4 which may be referred to as a Unit Under Test (UUT). If the UUT fails and can be reworked then do so, otherwise it needs to be scraped. 

**Warning: never use a soldering iron to rework ceramic capacitors due to the thermal shock.**
    
Items used for test.

![ItemsUsedForTest](./14226,ItemsUsedForTest.jpg "RPUadpt Items Used For Test")


## Assembly check

After assembly check the circuit carefully to make sure all parts are soldered and correct, note that the device making is labeled on the schematic and assembly drawing.
    

## IC Solder Test

Check continuity between pin and pad by measuring the reverse body diode drop from 0V (aka ground) and all other IC pads not connected to 0V. This value will vary somewhat depending on what the pin does, but there is typically an ESD diode to ground or sometimes a body diode (e.g. open drain MOSFET), thus a value of .3V to .7V is valid to indicate a solder connection. Note the RS485 drivers will show high impedance on the differential lines, so skip those.


## Bias +5V and Check LDO Regulator

Apply a 5V current limited source (about 30mA*) to the +5V header pin. Check that the the MIC5205 linear regulator has 3.3V (use ICSP J8 pin2).  Check that the input current is for a blank MCU (e.g. less than 5mA). Turn off power.


```
{   "I_IN_BLANKMCU_mA":[2.1,],
    "LDO_V":[3.285,] }
```

## Set MCU Fuse

The MCU needs its fuses set, so a Makefile (e.g. "make fuse") is used to do that. Apply a 5V current limited source (about 30mA*) to the +5V header pin. There is not a bootloader, it just sets fuses.

Use the <https://github.com/epccs/RPUadpt/tree/master/Bootload> Makefile 

Check that the input current is suitable for 8Mhz at 3.3V. Disconnect the ICSP tool and measure the input current. Turn off power.

```
{   "I_IN_MCU_8MHZ_INTRN_mA":[4.0,]}
```


## Load CheckDTR Firmware

Plug a header (or jumper) onto the +5V pin so that IOREF is jumpered to +5V. Connect TX pin to IOREF to pull it up (the MCU normaly does this). Plug a CAT5 RJ45 stub with 100 Ohm RX, TX and DTR pair terminations. Increase the current limit to 50mA. Load (e.g. "make isp") CheckDTR firmware to verify DTR control is working:

Use the <https://github.com/epccs/RPUadpt/tree/master/CheckDTR> Makefile

Use the CheckDTR firmware to verify DTR control is working. The program loops through the test. It blinks the red LED to show which test number is setup. If it keeps repeating a test then that test has failed.

As the firmware loops the input current can be measured, it should have two distinct levels, one when the DTR pair is driven low and one when the DTR pair is not driven. The blinking LED leaves the DMM unsettled. Turn off power.

```
{   "DTR_HLF_LD_mA":[33.0,],
    "DTR_NO_LD_mA":[10.0,] }
```


##  Check Differential Bias

Plug a header (or jumper) onto the +5V pin so that IOREF is jumpered to  +5V, set current limit at 100mA. Connect TX pin to IOREF to pull it up (the MCU normally does this). Plug a CAT5 RJ45 stub with RX, TX and DTR pair terminations. The CheckDTR firmware will set TX_DE and RX_DE high. Now power up the +5V. Check that both RX and TX have 5V. Check that TX_DI has 3.3V (U2 pin 4) and its TX_DE control (U2 pin 3) is pulled low. Check that HOST_TX has 3.3V (J3 pin 4).  Check that RX_DE (U6 pin 3) is pulled low. 

Now disconnect TX from IOREF and connect it to 0V, to simulate the MCU sending data. Check  that the input current is cycling between 56mA and 33mA. At 56mA the TX driver is driving the TX pair with half load and DTR driver is driving the DTR pair with a half load, while ony the TX pair is driven at 33mA. Turn off power.

```
{   "DTR_TX_HLF_LD_mA":[55.0,],
    "TX_HLF_LD_mA":[32.3,] }
```


## Differential Loopback with TX Driver

Plug a header (or jumper) onto the +5V pin so that IOREF is jumpered to +5V, set current limit at 100mA. Connect TX pin to 0V to pull it low. Plug a CAT5 RJ45 stub with RX, TX and DTR pair terminations. The CheckDTR firmware will set TX_DE and RX_DE high. Plug in a RJ45 loopback connector to connect the TX differential pair to the RX differential pair. Now power up the +5V and measure its input current. The TX driver is now driving a differential pair with 50 Ohms on it, which is the normal load. Verify that RX has 0V on it now. Turn off power.

```
{   "DTR_HLF_LD_TX_FL_LD_mA":[72.3,],
    "TX_FL_LD_mA":[49.0,] }
```


## Differential Loopback with RX Driver

Disconnect TX from ground and Connect it to IOREF, which will disable the TX driver (U2) so that the RX driver (U6) can operate through the RJ45 loopback. Plug in the RJ45 loopback connector so the TX pair is looped back to the RX pair. Short HOST_TX (J3 pin 4) to ground to cause the RX driver to drive the RX pair. Measure the supply current when RX is driven and when a DTR half load is added.

```
{   "DTR_HLF_LD_RX_FL_LD_mA":[73.0,],
    "RX_FL_LD_mA":[50.5,] }
```


## Load i2c-debug into an MCU board

Plug the RPUftdi into a bare metal MCU board (e.g. RPUno). Connect a USB to the RPUftdi, which should prvide 3.3V for RPUftdi bus manager. Connect the ICSP tool to the RPUftdi bus manager and load (e.g. "make isp") the Host2Remote firmware. 

Use the <https://github.com/epccs/RPUftdi/tree/master/Host2Remote> Makefile

Use picocom to connect, and verify that the red MNG_LED blinks. The DUT's red MNG_LED blinks when the UART pulls DTR active. Exit picocom with C-a, C-x. The MNG_LED should stop blinking after picocom exits.

Now load (e.g. "make bootload") the i2c-debug firmware onto the bare metal MCU board through the RPUftdi. 

Use the <https://github.com/epccs/RPUno/tree/master/i2c-debug> Makefile

Connect with picocom and set the byte that is sent when DTR/RTS toggles ('1' is 0x31 or 49).

``` 
picocom -b 115200 /dev/ttyUSB0
...
/0/address 41
{"address":"0x29"}
/0/buffer 3,49
{"txBuffer":[{"data":"0x3"},{"data":"0x31"}]}
/0/read? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x31"}]}
``` 

Exit picocom with C-a, C-x. This is now the bootload address that is broadcast when avrdude runs.

Plug the DUT (e.g. RPUadpt) into a second bare metal MCU board (e.g. RPUno). Connect power to the MCU board (e.g. RPUno can be powered with 5V or a 12V SLA and source that simulates 36 cell PV), which should prvide 5V for the regulator that powers the RPUadpt bus manager. Connect the ICSP tool to the RPUadpt bus manager and load (e.g. "make isp") the Remote firmware. 

Use the <https://github.com/epccs/RPUadpt/tree/master/Remote> Makefile

Now load (e.g. "make bootload") the i2c-debug firmware onto the bare metal MCU board through the RPUftdi.

Use the <https://github.com/epccs/RPUno/tree/master/i2c-debug> Makefile


## I2C Slave test

With i2c-debug loaded in the DUT's MCU board (e.g. an RPUno) power cycle the DUT board (e.g. disconnect PV and battery from its RPUno). If BUILTIN_LED is connected it will blink at 0.5Hz (1 sec on, and 1 sec off) when I2C reads correctly.

Note: hotplugging can damage the PV.

For reference, the RPUadpt shield address defaults to '1' and is changed with i2c-debug. Also the i2c-debug program reads the address during setup, so it will need a reset, but first I need to change the broadcast address to '2', so the DUT wil reset it's MCU board.

```
rsutherland@conversion:~/Samba/RPUadpt$ picocom -b 115200 /dev/ttyUSB0
...
/1/id?
{"id":{"name":"I2Cdebug","desc":"RPUno Board /w atmega328p and LT3652","avr-gcc":"4.9"}}
/1/scan?
{"scan":[{"addr":"0x29"}]}
/1/address 41
{"address":"0x29"}
/1/buffer 1,50
{"txBuffer":[{"data":"0x1"},{"data":"0x32"}]}
/1/read? 2
{"rxBuffer":[{"data":"0x1"},{"data":"0x32"}]}
/0/address 41
{"address":"0x29"}
/0/buffer 3,50
{"txBuffer":[{"data":"0x3"},{"data":"0x32"}]}
/0/read? 2
{"rxBuffer":[{"data":"0x3"},{"data":"0x32"}]}
```

Exit picocom with C-a, C-x, then start it again to broadcast address '2', which will cause the DUT to reset it's MCU board and the i2c-debug program will read the address.

``` 
picocom -b 115200 /dev/ttyUSB0
...
/2/address 41
{"address":"0x29"}
/2/buffer 0,255
{"txBuffer":[{"data":"0x0"},{"data":"0xFF"}]}
/2/read? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0x32"}]}
``` 