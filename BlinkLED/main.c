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

    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.

    sei(); // Enable global interrupts to start TIMER0

    while (1) 
    {
        unsigned long kRuntime = 0;

        digitalWrite(LED_BUILTIN,HIGH);
        blink_started_at = millis();
        while (kRuntime <= 100) // wait for 1/10 second
        {
            kRuntime = millis() - blink_started_at;
        }
        digitalWrite(LED_BUILTIN,LOW);
        _delay_ms(1000);
    }    
}

