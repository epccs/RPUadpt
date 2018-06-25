# BlinkLED

## Overview

Blink the on board LED plus debug the host I2C interface (e.g. the secnond I2C port on ATmega328pb).

The [i2c-debug] firmware on an RPUno can be used to test the I2C host manager interface, its address is at 0x2A. The RPUno controler has its own manager interfac at 0x29. 

[i2c-debug]: https://github.com/epccs/RPUno/tree/master/i2c-debug

'''
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
#blinking has stopped
/0/ibuff 0
{"txBuffer[2]":[{"data":"0x0"}]}
# SMBus seems to include the command byte automaticly for the next transaction
/0/iread? 2
{"rxBuffer":[{"data":"0x0"},{"data":"0xFC"}]}
#blinking has resumed
'''

SMBus (???do not asume I have done this is right???): After write_i2c_block_data the 328pb holds the old data. When read_i2c_block_data occures it will put a I2C packet with the address followed by a request to read (which is the active transaction), at whcih point the slave returns the data for the previous write (e.g. the previous transaction). Now have a look at the Python program for a Raspery Pi.

[i2c-debug]:(https://github.com/epccs/RPUadpt/tree/master/BlinkLED/toggle.py)
