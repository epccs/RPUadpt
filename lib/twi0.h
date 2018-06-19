/*
AVR TWI/I2C C library 
Copyright (C) 2018 Ronald Sutherland

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

For a copy of the GNU General Public License use
http://www.gnu.org/licenses/gpl-2.0.html

Original 2006 Nicholas Zambetti 
Modified in 2012 by Todd Krein (todd@krein.org) to implement repeated starts
Modified in 2016 by Ronald Sutherland (ronald.sutherlad@gmail) to use as a C library with avr-libc dependency
*/

#ifndef twi0_h
#define twi0_h

#ifndef TWI0_FREQ
#define TWI0_FREQ 100000L
#endif

#ifndef TWI0_BUFFER_LENGTH
#define TWI0_BUFFER_LENGTH 32
#endif

#define TWI0_READY 0
#define TWI0_MRX   1
#define TWI0_MTX   2
#define TWI0_SRX   3
#define TWI0_STX   4
  
void twi0_init(uint8_t);
void twi0_disable(void);
void twi0_setAddress(uint8_t);
void twi0_setFrequency(uint32_t);
uint8_t twi0_readFrom(uint8_t, uint8_t*, uint8_t, uint8_t);
uint8_t twi0_writeTo(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t twi0_transmit(const uint8_t*, uint8_t);
void twi0_attachSlaveRxEvent( void (*)(uint8_t*, int) );
void twi0_attachSlaveTxEvent( void (*)(void) );
void twi0_reply(uint8_t);
void twi0_stop(void);
void twi0_releaseBus(void);

#endif // twi0_h

