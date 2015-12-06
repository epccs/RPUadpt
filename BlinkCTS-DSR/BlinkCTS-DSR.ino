/*
  BlinkCTS-DSR on RPUadpt /w ATtiny1634 using arduino-tiny-841 core
  Turns FTDI_nCTS line high for 1/10 sec then FTDI_nDSR for 1/10 sec then off for 8/10 sec.
  

  on RPUadpt the FTDI_nCTS wire is PC0 (pin 13 with arduino-tiny-841 core),
  and the FTDI_nDSR wire is PA5 (pin 5 with arduino-tiny-841 core).


  modified 16 Nov 2015
  by Ronald Sutherland
 */

static const uint8_t LED = 4; 
static const uint8_t FTDI_nCTS = 13; 
static const uint8_t FTDI_nDSR = 5; 


void setup() {
  // LED pin is always low, it blinks by togling between INPUT and OUTPUT.
  digitalWrite(LED, LOW);
  pinMode(FTDI_nCTS, OUTPUT);
  digitalWrite(FTDI_nCTS, HIGH); // When an MCU is ready to receive data, it drives the TTL CTS low
  pinMode(FTDI_nDSR, OUTPUT);
  digitalWrite(FTDI_nDSR, HIGH);
}

void loop() {
  pinMode(LED, OUTPUT);      // turn the LED on by sinking current
  delay(100);                // wait for 1/10 second
  pinMode(LED, INPUT);       // turn the LED off by making pin high impedance
  digitalWrite(FTDI_nCTS, LOW);
  delay(100);
  digitalWrite(FTDI_nCTS, HIGH);
  digitalWrite(FTDI_nDSR, LOW);
  delay(100);
  digitalWrite(FTDI_nDSR, HIGH);
  delay(700);                // wait for 7/10 second
}
