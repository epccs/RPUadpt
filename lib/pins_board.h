/*
 * RPUadpt Pin definitions for use with DigitalIO library
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
 * along with the DigitalIO Library.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef Pins_Board_h
#define Pins_Board_h

#define NUM_DIGITAL_PINS            20
#define NUM_ANALOG_INPUTS        8
// analogInputToDigitalPin() takes an AVR analog channel number and returns the digital pin number otherwise -1.
//#define analogInputToDigitalPin(p)  ((p < 8) ? (p) + 14 : -1)
#define digitalPinHasPWM(p)         ((p) == 3 || (p) == 5 || (p) == 6 || (p) == 9 || (p) == 10 || (p) == 11)

#define DTR_RXD 0
#define DTR_TXD 1
#define HOST_nDTR 2
#define HOST_nRTS 3
#define RX_nRE 4
#define TX_DE 5
#define DTR_nRE 6
#define DTR_DE 7
#define ICP1 8

#define LED_BUILTIN 9

#define nSS 10
#define MOSI 11
#define MISO 12
#define SCK 13

#define HOST_nCTS 14
#define HOST_nDSR 15
#define TX_nRE 16
#define RX_DE 17

#define SDA 18
#define SCL 19

// these are ADC channels, they do not have digital IO function on ATmega328p
#define IOREF_ADC6 6
#define VIN_ADC7 7

#endif // Pins_Board_h
