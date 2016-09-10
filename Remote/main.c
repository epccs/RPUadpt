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
#include "../lib/twi.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

//slave address, use numbers between 0x08 to 0x78
#define TWI_ADDRESS 0x29
// 0x31 is ascii char for the letter '1', which may be used as second char of command e.g. /1/id?
#define RPU_ADDRESS '1'  
// default address sent on DTR pair if FTDI_nDTR toggles
#define RPU_HOST_CONNECT '0' 
// byte sent on DTR pair when FTDI_nDTR is no longer active
#define RPU_HOST_LOCKED ~RPU_HOST_CONNECT
// return to normal mode address sent on DTR pair (haha... no it is not oFF it is a hex value)
#define RPU_NORMAL_MODE 0xFF

static uint8_t rxBuffer[TWI_BUFFER_LENGTH];
static uint8_t rxBufferLength = 0;
static uint8_t txBuffer[TWI_BUFFER_LENGTH];
static uint8_t txBufferLength = 0;

#define BOOTLOADER_ACTIVE 2000
#define BLINK_BOOTLD_DELAY 75
#define BLINK_ACTIVE_DELAY 500
#define BLINK_LOCKOUT_DELAY 2000
#define BLINK_ERROR_DELAY 200
#define UART_TTL 500
#define LOCKOUT_DELAY 30000

static unsigned long blink_started_at;
static unsigned long lockout_started_at;
static unsigned long uart_started_at;
static unsigned long bootloader_started_at;

static uint8_t bootloader_started;
static uint8_t host_active;
static uint8_t bootloader_address; 
static uint8_t activate_lockout;
static uint8_t uart_has_TTL;
static uint8_t controlled_by_remote;

volatile uint8_t error_status;
volatile uint8_t uart_output;

// called when I2C data is received. 
void receiveEvent(uint8_t* inBytes, int numBytes) 
{
    // I have to save the data
    for(uint8_t i = 0; i < numBytes; ++i)
    {
        rxBuffer[i] = inBytes[i];    
    }
    rxBufferLength = numBytes;
    
    // This will copy the data to the txBuffer, which will then echo back with slaveTransmit()
    for(uint8_t i = 0; i < numBytes; ++i)
    {
        txBuffer[i] = inBytes[i];    
    }
    txBufferLength = numBytes;
    if (txBufferLength > 1)
    {
        if ( (txBuffer[0] == 0) ) // TWI command to read RPU_ADDRESS
        {
            txBuffer[1] = RPU_ADDRESS; // '0' is 0x31
        }
        if ( (txBuffer[0] == 2) ) // read byte sent on DTR pair if HOST_nDTR toggles
        {
            txBuffer[1] = bootloader_address;
        }
        if ( (txBuffer[0] == 3) ) // buffer the byte that is sent on DTR pair for use when HOST_nDTR toggles
        {
            bootloader_address = txBuffer[1];
        }
        if ( (txBuffer[0] == 5) ) // send RPU_NORMAL_MODE on DTR pair
        {
            if ( !(uart_has_TTL) )
            {
                // send a byte to the UART output
                uart_started_at = millis();
                uart_output = RPU_NORMAL_MODE;
                printf("%c", uart_output); 
                uart_has_TTL = 1;
            }
        }
        if ( (txBuffer[0] == 6) ) // TWI command to read error status
        {
            txBuffer[1] = error_status;
        }
        if ( (txBuffer[0] == 7) ) // TWI command to set/clear error status
        {
            error_status = txBuffer[1];
        }
    }
}

// called when I2C data is requested.
void slaveTransmit(void) 
{
    // respond with an echo of the last message sent
    uint8_t return_code = twi_transmit(txBuffer, txBufferLength);
    if (return_code != 0)
        error_status &= (1<<1); // bit one set 
}

void setup(void) 
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    pinMode(HOST_nRTS, INPUT);
    digitalWrite(HOST_nRTS, HIGH); // with AVR when the pin DDR is set as an input setting pin high will trun on a weak pullup 
    pinMode(HOST_nCTS, OUTPUT);
    digitalWrite(HOST_nCTS, LOW);
    pinMode(HOST_nDTR, INPUT);
    digitalWrite(HOST_nDTR, HIGH); // another weak pullup 
    pinMode(HOST_nDSR, OUTPUT);
    digitalWrite(HOST_nDSR, LOW);
    pinMode(RX_DE, OUTPUT);
    digitalWrite(RX_DE, HIGH);  // allow RX pair driver to enable if FTDI_TX is low
    pinMode(RX_nRE, OUTPUT);
    digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
    pinMode(TX_DE, OUTPUT);
    digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
    pinMode(TX_nRE, OUTPUT);
    digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    pinMode(DTR_DE, OUTPUT);
    digitalWrite(DTR_DE, LOW);  // seems to be a startup glitch ??? so disallow DTR pair driver to enable if DTR_TXD is low
    pinMode(nSS, OUTPUT); // nSS is input to a Open collector buffer used to pull to MCU nRESET low
    digitalWrite(nSS, HIGH); 

    bootloader_address = RPU_HOST_CONNECT; 
    host_active = 0;
    activate_lockout = 0;
    uart_has_TTL = 0;
    error_status = 0;

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);

    twi_setAddress(TWI_ADDRESS);
    twi_attachSlaveTxEvent(slaveTransmit); // called when I2C data is requested 
    twi_attachSlaveRxEvent(receiveEvent); // slave receive
    twi_init(false); // do not use internal pull-up

    sei(); // Enable global interrupts to start TIMER0 and UART
    
    _delay_ms(50); // wait for UART glitch to clear
    digitalWrite(DTR_DE, HIGH);  // allow DTR pair driver to enable if DTR_TXD is low
}

// blink if the host is active, fast blink if error status, slow blink in lockout
void blink_on_activate(void)
{
    unsigned long kRuntime = millis() - blink_started_at;
    
    if (!error_status) 
    {
        if ( bootloader_started  && (kRuntime > BLINK_BOOTLD_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_BOOTLD_DELAY; 
        }
        else if ( activate_lockout  && (kRuntime > BLINK_LOCKOUT_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_LOCKOUT_DELAY; 
        }
        else if ( host_active  && (kRuntime > BLINK_ACTIVE_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_ACTIVE_DELAY; 
        }
        // else spin the loop
    }
    else
    {
        if ( (kRuntime > BLINK_ERROR_DELAY))
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_ERROR_DELAY; 
        }
    }
}

void check_Bootload_Time(void)
{
    if (bootloader_started)
    {
        unsigned long kRuntime = millis() - bootloader_started_at;
        
        if ( kRuntime > BOOTLOADER_ACTIVE)
        {
            host_active =1;
            bootloader_started = 0;
        }
    }
}

void check_DTR(void)
{
    if (!controlled_by_remote)
    {
        if ( !digitalRead(HOST_nDTR) )  // both HOST_nDTR and HOST_nRTS are set (active low) when avrdude tries to use the bootloader
        { 
            if ( !(bootloader_started  || activate_lockout || host_active || uart_has_TTL) )
            {
                // send a byte on the DTR pair when FTDI_nDTR is first active
                uart_started_at = millis();
                uart_output= bootloader_address; // set by I2C, default is RPU_HOST_CONNECT
                printf("%c", uart_output); 
                uart_has_TTL = 1;
            }
        }
        else
        {
            if ( host_active && (!uart_has_TTL) && (!bootloader_started) && (!activate_lockout) )
            {
                // send a byte on the DTR pair when FTDI_nDTR is first non-active
                uart_started_at = millis();
                uart_output= RPU_HOST_LOCKED;
                printf("%c", uart_output); 
                uart_has_TTL = 1;
                digitalWrite(LED_BUILTIN, HIGH);
            }
        }
    }
}

// lockout needs to happoen for a long enough time to insure bootloading is finished,
void check_lockout(void)
{
    unsigned long kRuntime = millis() - lockout_started_at;
    
    if ( activate_lockout && (kRuntime > LOCKOUT_DELAY))
    {
        activate_lockout =0;
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if HOST_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to HOST_RX input
        
        host_active = 1;
    }
}

void check_uart(void)
{
    unsigned long kRuntime = millis() - uart_started_at;
 
    if ( uart0_available() )
    {
        char input;
        input = getchar();

        // was this byte sent with the local DTR pair driver, if so the error status may need update
        // and the lockout from a local host needs to be treated differently
        // need to ignore the local host's DTR if getting control from a remote host
        if ( uart_has_TTL )
        {
            if(input != uart_output) 
            { // sent byte does not match,  but I'm not to sure what would cause this.
                error_status &= (1<<2); // bit two set 
            }
            uart_has_TTL = 0;
            controlled_by_remote = 0;
        }
        else
        {
            controlled_by_remote = 1;
        }

        if (input == RPU_NORMAL_MODE) // end the lockout if it was set.
        { 
            activate_lockout =0;
            host_active =1;
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if HOST_TX is low
            digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
            digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to HOST_RX input
        }
        else if (input == RPU_HOST_LOCKED) // the host is disconnected 
        { 
            activate_lockout =0;
            host_active =0;
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
            digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
            digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior that outputs to FTDI_RX input
        }
        else
        {
            if (input == RPU_ADDRESS) // that is my local address
            {
                // connect the host and local mcu
                digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
                digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
                digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
                digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input

                // start the bootloader
                bootloader_started = 1;
                digitalWrite(nSS, LOW);   // nSS goes through a open collector buffer to nRESET
                _delay_ms(20);  // hold reset low for a short time 
                digitalWrite(nSS, HIGH); // this will release the buffer with open colllector on MCU nRESET.
                blink_started_at = millis();
                bootloader_started_at = millis();
            }
            else
            {  // lockout both MCU and host
                activate_lockout =1;
                digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if HOST_TX is low
                digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
                digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
                digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to HOST_RX input
                lockout_started_at = millis();
                blink_started_at = millis();
            }
        }
    }
    else if (uart_has_TTL && (kRuntime > UART_TTL) )
    { // perhaps the DTR line is stuck (e.g. someone has pulled it low) so may need to time out
        error_status &= (1<<0);
        uart_has_TTL = 0;
    }
}

int main(void)
{
    setup();

    blink_started_at = millis();

    while (1) 
    {
        blink_on_activate();
        check_Bootload_Time();
        check_DTR();
        check_lockout();
        check_uart();
    }    
}

