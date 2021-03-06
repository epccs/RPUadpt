/* DigitalIO Library
 * Copyright (C) 2016 Ronald Sutherland
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the DigitalIO Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Hacked from William Greiman to work in C with my board
 * Functions are inspired by Wiring from Hernando Barragan
 */
#ifndef PinNum_h
#define PinNum_h

// avr-libc
#include <avr/io.h>
#include <util/atomic.h>

// avr-gcc
#include <stdbool.h>

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

typedef struct {
  volatile uint8_t* ddr; 
  volatile uint8_t* pin; 
  volatile uint8_t* port;
  uint8_t bit;  
} Pin_Map;

// the device-specs provide the define, so use "avr-gcc -B" to provide one for the 328pb if it is not in mainline
#if defined(__AVR_ATmega328PB__)

#define NUM_DIGITAL_PINS 24

/* Each of the AVR Digital I/O ports is associated with three I/O registers. 
8 bit Data Direction Register (DDRx) each bit sets a pin as input (=0) or output (=1).
8 bit Port Input Register (PINx) each  bit is the input from a pin that was latched durring last low edge of the system clock.
8 bit Port Data Register (PORTx) each bit drives a pin if set as output (or sets pullup if input)
Where x is the port A, B, C, etc.

Wiring uses pin numbers to control their functions. */
static const Pin_Map pinMap[NUM_DIGITAL_PINS] = {
    [0] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD0 }, // DTR_RXD
    [1] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD1 }, // DTR_TXD
    [2] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD2 }, // HOST_nDTR
    [3] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD3 }, // HOST_nRTS
    [4] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD4 }, // RX_nRE
    [5] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD5 }, // TX_DE
    [6] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD6 }, // DTR_nRE
    [7] = { .ddr=&DDRD, .pin=&PIND, .port=&PORTD, .bit= PD7 }, // DTR_DE
    [8] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB0 }, // SHUTDWN 
    [9] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB1 }, // LED_BUILTIN
    [10] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB2 }, // nSS
    [11] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB3 }, // MOSI
    [12] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB4 }, // MISO
    [13] = { .ddr=&DDRB, .pin=&PINB, .port=&PORTB, .bit= PB5 }, // SCK
    [14] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC0 }, // HOST_nCTS
    [15] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC1 }, // HOST_nDSR
    [16] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC2 }, // TX_nRE
    [17] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC3 }, //  RX_DE
    [18] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC4 }, // SDA0
    [19] = { .ddr=&DDRC, .pin=&PINC, .port=&PORTC, .bit= PC5 }, //  SCL0
    [20] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE2 }, // 
    [21] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE3 }, // 
    [22] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE0 }, // SDA1
    [23] = { .ddr=&DDRE, .pin=&PINE, .port=&PORTE, .bit= PE1 } // SCL1
};
#endif  // defined(__AVR_ATmega328PB__)

void badPinNumber(void) __attribute__((error("Pin number is too large or not a constant")));

static inline __attribute__((always_inline)) void badPinCheck(uint8_t pin) 
{
    if (pin >= NUM_DIGITAL_PINS) badPinNumber();
}

static inline __attribute__((always_inline))
void bitWrite(volatile uint8_t* register_addr, uint8_t bit_offset, bool value_for_bit) 
{
    // Although I/O Registers 0x20 and 0x5F, e.g. PORTn and DDRn should not need 
    // atomic change control it does not harm.
    ATOMIC_BLOCK ( ATOMIC_RESTORESTATE )
    {
        if (value_for_bit) 
        {
            *register_addr |= 1 << bit_offset;
        } 
        else 
        {
            *register_addr &= ~(1 << bit_offset);
        }
    }
}

/* read value from pin number */
static inline __attribute__((always_inline))
bool digitalRead(uint8_t pin_num) 
{
    badPinCheck(pin_num);
    return (*pinMap[pin_num].pin >> pinMap[pin_num].bit) & 1;
}

/* set pin value */
static inline __attribute__((always_inline))
void digitalWrite(uint8_t pin_num, bool value_for_bit) {
    badPinCheck(pin_num);
    bitWrite(pinMap[pin_num].port, pinMap[pin_num].bit, value_for_bit);
}

/* toggle pin*/
static inline __attribute__((always_inline))
void digitalToggle(uint8_t pin) {
    badPinCheck(pin);
    // Ckeck if pin is in OUTPUT mode befor changing it
    if( ( ( (*pinMap[pin].ddr) >> pinMap[pin].bit ) & 1) == OUTPUT )  
    {
        digitalWrite(pin, !digitalRead(pin));
    }
}

/* set pin mode INPUT and OUTPUT */
static inline __attribute__((always_inline))
void pinMode(uint8_t pin_num, bool output_mode) {
    badPinCheck(pin_num);
    bitWrite(pinMap[pin_num].ddr, pinMap[pin_num].bit, output_mode);
}

#endif  // DigitalPin_h

