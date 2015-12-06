/*
 * StudioBlink.cpp
 *
 * For Testing RPUadpt ^1+
 * low_fuses=0xE2
 * high_fuses=0xD5
 * extended_fuses=0x1D (reads as 0xFD since unused bits don't set)
 
 * Created: 9/28/2015 7:43:58 PM
 * Author : rsutherland
 */ 

#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <avr/wdt.h> // http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
#include "pin_macros.h"


int main(void)
{
	// RPUadpt ^1 has a red LED on PA4 connected to 5V thru 1.5k Ohm
	// The pin has an ESD diode to its supply (3V3) and that plus the LED 
    // voltage keeps the LED off, however setting the pin as an output at 3V3 
    // does not turn off the LED since it is tied to 5V through the 1.5k resistor.
    
	// set the state of PA4 low (it will only go low when set as an output)
	a4_low;
	
	// set PC2 as an input, it is connected to FTDI_nDTR
	c2_input;
	// warning PB3 is not a reliable input for FTDI_nRTS unless I turn on watchdog timer (from tiny1634 errata for rev B)
	b3_input;
	
	wdt_enable(WDTO_8S);
	
    while (1) 
    {
		if(b3_read && c2_read) //PB3 is FTDI_nRTS and PC2 is FTDI_nDTR
        { 
			a4_output; // Turn the led on by setting it as an output.
		} 
        else 
        {
			a4_input; // Turn led off by setting it as an input.
		}
        /* most basic form of blink
        a4_output;
        _delay_ms(1000);
        a4_input;
        _delay_ms(1000);
        */
		wdt_reset();
    }    
}

