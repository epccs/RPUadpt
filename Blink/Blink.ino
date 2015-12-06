/*
   Blink on RPUadpt /w ATtiny1634 using arduino-tiny-841 core
   Turns on an LED on for one-tenth second, then off for nine-tenth
   second, repeatedly.

  RPUadpt has an LED on PA4 (pin 4 with arduino-tiny-841 core) that you can control.
  The led is connected to 5V through a 1.5k (^1+) resistor. 
  The pin has an ESD diode to its supply (3V3) and that plus the LED 
  voltage keeps the LED off, however setting the pin as an output at 3V3 
  does not turn off the LED since it is tied to 5V through the 1.5k resistor.

  modified 16 Nov 2015
  by Ronald Sutherland
 */


void setup() {
  // LED pin is always low, it blinks by togling between INPUT and OUTPUT.
  digitalWrite(4, LOW);
}

void loop() {
  pinMode(4, OUTPUT);      // turn the LED on by sinking current
  delay(100);              // wait for 1/10 second
  pinMode(4, INPUT);       // turn the LED off by making pin high impedance
  delay(900);              // wait for 9/10 second
}
