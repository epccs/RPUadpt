/*
  BlinkCTS-DSR on RPUadpt /w ATtiny1634 using arduino-tiny-841 core
  Turns FTDI_nCTS line high for 1/10 sec then FTDI_nDSR for 1/10 sec then off for 8/10 sec.
  

  on RPUadpt the FTDI_nCTS wire is PC0 (pin 13 with arduino-tiny-841 core),
  and the FTDI_nDSR wire is PA5 (pin 5 with arduino-tiny-841 core).
    note: pin 5 does not work as output with arduino-tiny-841 core, I need to research this
    but I think the pin macros may be better at this than a core will ever be.


  by Ronald Sutherland
 */

#include <avr/wdt.h> // http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html

#include "adpt_pin_macros.h"
/* macros include [name]_input, [name]_output, [name]_high, [name]_low, [name]_read,
 * PA4: is LED, e.g. LED_input, LED_output, LED_low...
 * PA5: is FTDI_nDSR
 * PB3: is FTDI_nRTS
 * PC0: is FTDI_nCTS
 * PC0: is FTDI_nDTR
 */ 


void setup() {
  LED_low; // LED pin is always low, it blinks by togling between INPUT and OUTPUT.
  FTDI_nCTS_output;
  FTDI_nCTS_high; // When an MCU is ready to receive data, it drives the TTL CTS low (so MCU is not ready)
  FTDI_nDSR_output;
  FTDI_nDSR_high;
  FTDI_nRTS_input; //it defaults to this but I want to show it anyway
  FTDI_nDTR_input;
  // wdt_enable(WDTO_8S); // this allows use of FTDI_nRTS (PB3) on ATtinny1634 as input
}

void loop() {
  LED_output;      // turn the LED on by sinking current
  delay(100);                // wait for 1/10 second
  LED_input;       // turn the LED off by making pin high impedance
  FTDI_nCTS_low;
  delay(100);
  FTDI_nCTS_high;
  FTDI_nDSR_low;
  delay(100);
  FTDI_nDSR_high;
  delay(700);                // wait for 7/10 second
  // wdt_reset(); // agian this allows use of FTDI_nRTS (PB3) on ATtinny1634 as input
}
