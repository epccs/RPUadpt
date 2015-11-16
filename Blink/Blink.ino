/*
  Blink
  Turns on an LED on for one half second, then off for one 
  half second, repeatedly.

  RPUadpt has an LED you can control.

  This example code is in the public domain.

  modified 16 Nov 2015
  by Ronald Sutherland
 */


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital LED pin as an output.
  pinMode(4, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(4, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);              // wait for a second
  digitalWrite(4, LOW);    // turn the LED off by making the voltage LOW
  delay(500);              // wait for a second
}
