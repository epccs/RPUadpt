#!/usr/bin/env python3
#Use Raspberry Pi /dev/i2c-1

# notes: https://pypi.org/project/smbus2/
# https://github.com/quick2wire/quick2wire-python-api
# py-smbus is not active but has some forks https://github.com/pimoroni/py-smbus

import smbus

# on a Raspberry Pi with 256k memory use 0 for /dev/i2c-0
bus = smbus.SMBus(1)

RPUBUS_MGR_I2C_ADDR = 0x2A
I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE = 0
data = [3]

#write the command
bus.write_i2c_block_data(RPUBUS_MGR_I2C_ADDR, I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE, data)

#Read a block of bytes from address 
offset =0
# up to 32 bytes can be buffered, offset allows reading deeper but burns up bandwidth
num_of_bytes = len( [I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE] ) + len (data)
# the command is included in the echo

echo = bus.read_i2c_block_data(RPUBUS_MGR_I2C_ADDR, offset, num_of_bytes)
print(echo)

# My first attempt had an echo of [0, 255],  but where was that from. 
# SMBus is different than the way I was using bare metal I2c. 
# The SMBus write block is an I2C transaction and causes a receiving event. 
# The SMBus read block is a second I2C transaction and has another receiving event followed by a transmit event.
# My first attempt was data from the second transaction receiving event.
# I have to keep the data sent in the first receive event to echo it back in the second transaction transmit event.
# [0, 252]
