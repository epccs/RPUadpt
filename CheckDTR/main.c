/* RPUadpt CheckDTR for hardware testing
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
 *
 * story: first ^3 board had a little whisker of solder between DTR_RXD and DTR_TXD
 * three days was spent chasing the ghost.
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

static unsigned long blink_started_at;

void blink(uint8_t count)
{
    unsigned long kRuntime;
    
    for (uint8_t i=0 ;i<count; i++)
    {
        digitalToggle(LED_BUILTIN);
        kRuntime = 0;
        blink_started_at = millis();
        while (kRuntime <= 300) // wait for 1/10 second
        {
            kRuntime = millis() - blink_started_at;
        }
        digitalToggle(LED_BUILTIN);
        kRuntime = 0;
        blink_started_at = millis();
        while (kRuntime <= 300) // wait for 1/10 second
        {
            kRuntime = millis() - blink_started_at;
        }
    }
}

int main(void)
{
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN,LOW);
    
    // set DTR transceiver
    pinMode(DTR_TXD,OUTPUT);
    digitalWrite(DTR_TXD,LOW);
    pinMode(DTR_DE,OUTPUT);
    digitalWrite(DTR_DE,HIGH);
    pinMode(DTR_nRE,OUTPUT);
    digitalWrite(DTR_nRE,LOW);

    // set TX and RX transceiver 
    pinMode(TX_DE,OUTPUT);
    digitalWrite(TX_DE,HIGH);
    pinMode(RX_DE,OUTPUT);
    digitalWrite(RX_DE,HIGH);
    // note TX_nRE and RX_nRE each have a pull down and thus do not need changed
    
    initTimers(); //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.

    sei(); // Enable global interrupts to start TIMER0

    while (1) 
    {
        _delay_ms(4000);

        // set DTR transceiver DI input with 0V.
        digitalWrite(DTR_TXD,LOW);
        // set DTR transceiver DTR_DE HIGH to drive DTR pair, R12 has 3.3V.
        digitalWrite(DTR_DE,HIGH);
        // set DTR_nRE LOW cause transceiver to copy DTR pair to RO output
        digitalWrite(DTR_nRE,LOW);
        
        // DTR pair should be driven LOW by DI and copy its value to RO
        do
        {    
            blink(1);
            _delay_ms(1000);
        } while (digitalRead(DTR_RXD) != LOW);

        // set DTR transceiver DI input with 3.3V should cause an undriven pair
        digitalWrite(DTR_TXD,HIGH);

        // a High on DTR_TXD causes Q2 to pull down DTR_DE so the
        // DTR pair should be undriven which is seen at RO as a HIGH 
        do
        {    
            blink(2);
            _delay_ms(1000);
        } while(digitalRead(DTR_RXD) != HIGH);
 
        digitalWrite(DTR_DE,LOW);
        // set DTR_nRE HIGH to disconnect transceiver so R10 pulls up RO output
        digitalWrite(DTR_nRE,HIGH);
        digitalWrite(DTR_TXD,LOW);

        // DTR pair should be driven LOW but RO is disconnected and should pull-up to PASS
        do
        {    
            blink(3);
            _delay_ms(1000);
        } while (digitalRead(DTR_RXD) != HIGH);

        // set DTR_DE HIGH to copy DI onto DTR pair.
        digitalWrite(DTR_DE,HIGH);

        // DTR pair should not be driven and RO is disconnected and should pull-up to PASS
        do
        {    
            blink(4);
            _delay_ms(1000);
        } while (digitalRead(DTR_RXD) != HIGH);
    }    
}

