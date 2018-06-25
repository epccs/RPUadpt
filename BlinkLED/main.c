/* RPUadpt BlinkLED 
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
 * along with the Arduino DigitalIO Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/twi1.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

static uint8_t i2c1Buffer[TWI1_BUFFER_LENGTH];
static uint8_t i2c1BufferLength = 0;
static uint8_t i2c1_oldBuffer[TWI1_BUFFER_LENGTH];
static uint8_t i2c1_oldBufferLength = 0;

static unsigned long blink_started_at;

#define I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE 0
static uint8_t i2c_toggle;

// called when I2C slave has received data
// Clock streatching should have been enabled befor this event was called
void receive1_event(uint8_t* inBytes, int numBytes) 
{
    for(uint8_t i = 0; i < i2c1BufferLength; ++i)
    {
        i2c1_oldBuffer[i] = i2c1Buffer[i];    
    }
    i2c1_oldBufferLength = i2c1BufferLength;
    for(uint8_t i = 0; i < numBytes; ++i)
    {
        i2c1Buffer[i] = inBytes[i];    
    }
    i2c1BufferLength = numBytes;
    if (i2c1BufferLength > 1)
    {
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_TOGGLE_LED_BUILTIN_MODE) )
        {
            i2c_toggle = !i2c_toggle;
            i2c1Buffer[1] = ~i2c1Buffer[1]; // return inverted data... just to see it did somthing
        }
    }
}

// called when I2C slave has been requested to send data
// Clock streatching should have been enabled befor this event was called
void transmit1_event(void) 
{
    // SMBus seems to expect the old data from the last I2C packet so lets try that
    twi1_transmit(i2c1_oldBuffer, i2c1_oldBufferLength);
}

int main(void)
{
    i2c_toggle = 1;
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);      // turn the LED on by sinking current
    pinMode(SHUTDOWN, INPUT);
    digitalWrite(SHUTDOWN, HIGH); // trun on a weak pullup so the host computer does not shutdown


    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.

    twi1_setAddress(I2C1_ADDRESS);
    twi1_attachSlaveTxEvent(transmit1_event); // called when I2C slave has been requested to send data
    twi1_attachSlaveRxEvent(receive1_event); // called when I2C slave has received data
    twi1_init(false); // do not use internal pull-up
    
    sei(); // Enable global interrupts to start TIMER0
    
    blink_started_at = millis();

    while (1) 
    {
        unsigned long kRuntime = 0;
        
        kRuntime = millis() - blink_started_at;
        
        if (kRuntime > 1000) 
        {
            if (i2c_toggle) digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += 1000; 
        }
    }    
}

