/*
 * StudioBlink.cpp
 *
 * Created: 9/28/2015 7:43:58 PM
 * Author : rsutherland
 */ 

#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/io.h>


int main(void)
{
	// RPUadpt ^1 has a red LED on PA4 connected to 5V thru 1.5k Ohm
	// the pin is 5V tolerant as an input but will 
	// still sink current if driven high with 3V3
	// so this will set the output low...
	PORTA &= ~(1<<PA4); // NOT gives 01111111, then the AND gives 0******* where * is no change
	
    /* loop */
    while (1) 
    {
		// Turn the led on by setting it as an output.
		DDRA |= (1<<PA4); // OR gives 1******* where * is no change
	
		_delay_ms(1000);
	
		// Turn led off by setting it as an input.
		DDRA &= ~(1<<PA4);
	
		_delay_ms(1000);
    }
}

