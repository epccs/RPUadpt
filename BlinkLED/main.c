/* RPUadpt Blink
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
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

static unsigned long blink_started_at;

int main(void)
{
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);      // turn the LED on by sinking current

    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.

    sei(); // Enable global interrupts to start TIMER0
    
    blink_started_at = millis();

    while (1) 
    {
        unsigned long kRuntime = 0;
        
        kRuntime = millis() - blink_started_at;
        
        if (kRuntime > 1000) 
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += 1000; 
        }
    }    
}

