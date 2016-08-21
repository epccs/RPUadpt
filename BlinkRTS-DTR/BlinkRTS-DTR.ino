/*
  BlinkCTS-DSR on RPUadpt /w ATtiny1634 using arduino-tiny-841 core
  Turns FTDI_nCTS line high for 1/10 sec then FTDI_nDSR for 1/10 sec then off for 8/10 sec.
  

  on RPUadpt the FTDI_nRTS wire is PB3 (pin 14 with arduino-tiny-841 core),
  and the FTDI_nDTR wire is PC2 (pin 11 with arduino-tiny-841 core).

  avrdude arguments for ArduinoISP on AS7 as extern tool
   -F -v -pattiny1634 -cstk500v1 -e -PCOM3 -b19200 -D -Uflash:w:"$(BinDir)\$(TargetName).hex":i -C"E:/Program Files (x86)/Arduino/hardware/tools/avr/etc/avrdude.conf"

  by Ronald Sutherland
 */
 
#include <avr/wdt.h> // http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html

//are pin macros a better way?, They waste less rom and ram.
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
  wdt_enable(WDTO_8S); // this allows use of FTDI_nRTS (PB3) on ATtinny1634 as input
}

void loop() {
  //digitalRead of FTDI_nRTS (PB3) on ATtinny1634 only works if watchdog is enabled (see datasheet errata)
  if (FTDI_nRTS_read && FTDI_nDTR_read) 
  {
    LED_output;      // turn the LED on by sinking current
  } 
  else
  {
    LED_input;       // turn the LED off by making pin high impedance
  }
  /*
  digitalWrite(FTDI_nCTS, LOW);
  delay(100);
  digitalWrite(FTDI_nCTS, HIGH);
  digitalWrite(FTDI_nDSR, LOW);
  delay(100);
  digitalWrite(FTDI_nDSR, HIGH);
  delay(700);                // wait for 7/10 second
  */
  wdt_reset(); // agian this allows use of FTDI_nRTS (PB3) on ATtinny1634 as input
}
