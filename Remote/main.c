/* Remote is for ATMega328 bus manager on RPUadpt board
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
 */ 

#include <util/delay.h>
#include <avr/io.h>
#include "../lib/timers.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

#define BLINK_DELAY 500
#define LOCKOUT_DELAY 30000

static unsigned long blink_started_at;
static unsigned long lockout_started_at;

int main(void)
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(HOST_nRTS, INPUT);
    pinMode(HOST_nCTS, OUTPUT);
    digitalWrite(HOST_nCTS, LOW);
    pinMode(HOST_nDTR, INPUT);
    pinMode(HOST_nDSR, OUTPUT);
    digitalWrite(HOST_nDSR, LOW);
    pinMode(RX_DE, OUTPUT);
    digitalWrite(RX_DE, HIGH);  // allow RX pair driver to enable if HOST_TX is low
    pinMode(RX_nRE, OUTPUT);
    digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
    pinMode(TX_DE, OUTPUT);
    digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
    pinMode(TX_nRE, OUTPUT);
    digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to HOST_RX input
    pinMode(DTR_DE, OUTPUT);
    digitalWrite(DTR_DE, LOW);  // disallow DTR pair driver to enable if DTR_TXD is low
    pinMode(nSS, OUTPUT); // nSS is input to a Open collector buffer used to pull to MCU nRESET low
    digitalWrite(nSS, HIGH); 

    uint8_t activate_bootloader = 0;
    uint8_t activate_lockout = 0;

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);

    sei(); // Enable global interrupts to start TIMER0 and UART

    uart0_flush(); // dump the transmit buffer
    digitalWrite(DTR_DE, HIGH);  // allow DTR pair driver to enable if DTR_TXD is low

    blink_started_at = millis();

    while (1) 
    {
        unsigned long kRuntime = 0;
        
        kRuntime = millis() - blink_started_at;
        
        if ( activate_bootloader & (kRuntime > BLINK_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_DELAY; 
        }

        kRuntime = millis() - lockout_started_at;
        
        if ( activate_lockout & (kRuntime > LOCKOUT_DELAY))
        {
            activate_lockout =0;
            activate_bootloader =0;
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if HOST_TX is low
            digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
            digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to HOST_RX input
        }

        if ( uart0_available() )  // a byte from DTR pair may activate bootloader or cause lockout for a time
        { 
            char input = getchar();
            
            if (input == '1') // Host2Remote sends a '1' when its DTR is toggled, 
            {
                activate_bootloader =1;
                digitalWrite(nSS, LOW);   // nSS goes through a open collector buffer to nRESET
                _delay_ms(20);  // hold reset low for a short time 
                digitalWrite(nSS, HIGH); // this will release the buffer with open colllector on MCU nRESET.
                
                // set lockout flag to use its delay but do not disable the tranceivers
                activate_lockout =1;
                lockout_started_at = millis();
            }
            else
            {
                digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if HOST_TX is low
                digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
                digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
                digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to HOST_RX input
                activate_lockout =1;
                lockout_started_at = millis();
            }
        }
    }    
}

