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
	// RPUadpt ^1 has a red LED on PA4, pull down to turn on
	DDRA |= (1<<PA4);
	
    /* loop */
    while (1) 
    {
		// Turn the led on by setting corresponding register bit high.
		PORTA |= (1<<PA4); // or gives 1******* where * is no change
	
		_delay_ms(1000);
	
		// Turn led off by setting corresponding register bit low.
		PORTA &= ~(1<<PA4); // not gives 01111111, and gives 0*******
	
		_delay_ms(1000);
    }
}

