/*
AVR TWI/I2C C library 
Copyright (C) 2016 Ronald Sutherland

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

#ifndef twi1_h
#define twi1_h

#ifndef TWI1_FREQ
#define TWI1_FREQ 100000L
#endif

#ifndef TWI1_BUFFER_LENGTH
#define TWI1_BUFFER_LENGTH 32
#endif

#define TWI1_READY 0
#define TWI1_MRX   1
#define TWI1_MTX   2
#define TWI1_SRX   3
#define TWI1_STX   4
  
void twi1_init(uint8_t);
void twi1_disable(void);
void twi1_setAddress(uint8_t);
void twi1_setFrequency(uint32_t);
uint8_t twi1_readFrom(uint8_t, uint8_t*, uint8_t, uint8_t);
uint8_t twi1_writeTo(uint8_t, uint8_t*, uint8_t, uint8_t, uint8_t);
uint8_t twi1_transmit(const uint8_t*, uint8_t);
void twi1_attachSlaveRxEvent( void (*)(uint8_t*, int) );
void twi1_attachSlaveTxEvent( void (*)(void) );
void twi1_reply(uint8_t);
void twi1_stop(void);
void twi1_releaseBus(void);

#endif

