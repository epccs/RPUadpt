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
#include <avr/eeprom.h> 
#include <avr/pgmspace.h>
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
// byte broadcast on DTR pair when HOST_nDTR (or HOST_nRTS) is no longer active
#define RPU_HOST_DISCONNECT ~RPU_HOST_CONNECT
// return to normal mode address sent on DTR pair (haha... no it is not oFF it is a hex value)
#define RPU_NORMAL_MODE 0x00
// disconnect and set error status on DTR pair
#define RPU_ERROR_MODE 0xFF

// If the ID is matched in EEPROM then the rpu_address is taken from EEPROM
#define EE_RPU_IDMAX 10
const uint8_t EE_IdTable[] PROGMEM =
{
    'R',
    'P',
    'U',
    'a',
    'd',
    'p',
    't',
    '\0' // null term
};
#define EE_RPU_ID 40
#define EE_RPU_ADDRESS 50

static uint8_t rxBuffer[TWI_BUFFER_LENGTH];
static uint8_t rxBufferLength = 0;
static uint8_t txBuffer[TWI_BUFFER_LENGTH];
static uint8_t txBufferLength = 0;

#define BOOTLOADER_ACTIVE 25000
#define BLINK_BOOTLD_DELAY 75
#define BLINK_ACTIVE_DELAY 500
#define BLINK_LOCKOUT_DELAY 2000
#define BLINK_ERROR_DELAY 200
#define UART_TTL 500
#define LOCKOUT_DELAY 30000
#define SHUTDOWN_TIME 1000

static unsigned long blink_started_at;
static unsigned long lockout_started_at;
static unsigned long uart_started_at;
static unsigned long bootloader_started_at;
static unsigned long shutdown_started_at;

static uint8_t bootloader_started;
static uint8_t host_active;
static uint8_t localhost_active;
static uint8_t bootloader_address; 
static uint8_t lockout_active;
static uint8_t uart_has_TTL;
static uint8_t host_is_foreign;
static uint8_t local_mcu_is_rpu_aware;
static uint8_t rpu_address;
static uint8_t write_rpu_address_to_eeprom;
static uint8_t shutdown_detected;
static uint8_t shutdown_started;

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
            txBuffer[1] = rpu_address; // '1' is 0x31
            local_mcu_is_rpu_aware =1;
            
            // end the local mcu lockout. 
            if (localhost_active) 
            {
                // If the local host is active then broadcast on DTR pair
                uart_started_at = millis();
                uart_output = RPU_NORMAL_MODE;
                printf("%c", uart_output); 
                uart_has_TTL = 1; // causes host_is_foreign to be false
            }
            else 
                if (bootloader_started)
                {
                    // If the bootloader_started has not timed out yet broadcast on DTR pair
                    uart_started_at = millis();
                    uart_output = RPU_NORMAL_MODE;
                    printf("%c", uart_output); 
                    uart_has_TTL = 0; // causes host_is_foreign to be true, so local DTR/RTS is not accepted
                } 
                else
                {
                    lockout_started_at = millis() - LOCKOUT_DELAY;
                    bootloader_started_at = millis() - BOOTLOADER_ACTIVE;
                }
        }
        if ( (txBuffer[0] == 1) ) // set rpu_bus address and write it to eeprom
        {
            rpu_address = txBuffer[1];
            write_rpu_address_to_eeprom = 1;
        }
        if ( (txBuffer[0] == 2) ) // read byte sent on DTR pair if HOST_nDTR toggles
        {
            txBuffer[1] = bootloader_address;
        }
        if ( (txBuffer[0] == 3) ) // buffer the byte that is sent on DTR pair for use when HOST_nDTR toggles
        {
            bootloader_address = txBuffer[1];
        }
        if ( (txBuffer[0] == 4) ) // when ICP1 pin is pulled  down the host (Pi Zero on RPUp) should hault
        {
            txBuffer[1] = shutdown_detected;
            shutdown_detected = 0; // clear shutdown_detected, it is set in check_shutdown()
        }
        if ( (txBuffer[0] == 5) ) // pull ICP1 pin low to hault the host (Pi Zero on RPUp)
        {
            if (txBuffer[1] == 1)
            {
                pinMode(SHUTDOWN, OUTPUT);
                digitalWrite(SHUTDOWN, LOW);
                pinMode(LED_BUILTIN, OUTPUT);
                digitalWrite(LED_BUILTIN, HIGH);
                shutdown_started = 1; // it is cleared in check_shutdown()
                shutdown_detected = 0; // it is set in check_shutdown()
                shutdown_started_at = millis();
            }
            // else ignore
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

void connect_normal_mode(void)
{
    // connect the local mcu if it has talked to the rpu manager (e.g. got an address)
    if(host_is_foreign)
    {
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }

     // connect both the local mcu and host/ftdi uart if mcu is rpu aware, otherwise block MCU from using the TX pair
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        if(local_mcu_is_rpu_aware)
        {
            digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        }
        else
        {
            digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        }
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

void connect_bootload_mode(void)
{
    // connect the remote host and local mcu
    if (host_is_foreign)
    {
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // connect the local host and local mcu
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, LOW);  // enable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, HIGH); // allow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
}

void connect_lockout_mode(void)
{
    // lockout everything
    if (host_is_foreign)
    {
        digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior to output to FTDI_RX input
    }
    
    // lockout MCU, but not host
    else
    {
        digitalWrite(RX_DE, HIGH); // allow RX pair driver to enable if FTDI_TX is low
        digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
        digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
        digitalWrite(TX_nRE, LOW);  // enable TX pair recevior to output to FTDI_RX input
    }
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
    pinMode(SHUTDOWN, INPUT);
    digitalWrite(SHUTDOWN, HIGH); // trun on a weak pullup 

    bootloader_address = RPU_HOST_CONNECT; 
    host_active = 0;
    lockout_active = 0;
    error_status = 0;
    write_rpu_address_to_eeprom = 0;
    shutdown_detected = 0;
    shutdown_started = 0;

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
    
    // check if eeprom ID is valid
    uint8_t EE_id_valid = 0;
    for(uint8_t i = 0; i <EE_RPU_IDMAX; i++)
    {
        uint8_t id = pgm_read_byte(&EE_IdTable[i]);
        uint8_t ee_id = eeprom_read_byte((uint8_t*)(i+EE_RPU_ID)); 
        if (id != ee_id) 
        {
            EE_id_valid = 0;
            break;
        }
        
        if (id == '\0') 
        {
            EE_id_valid = 1;
            break;
        }
    }

    // Use eeprom value for rpu_address if ID was valid    
    if ( EE_id_valid )
    {
        rpu_address = eeprom_read_byte((uint8_t*)(EE_RPU_ADDRESS));
    }
    else
    {
        rpu_address = RPU_ADDRESS;
    }
    
    // is foreign host in control? (ask over the DTR pair)
    uart_has_TTL = 0;
}

// blink if the host is active, fast blink if error status, slow blink in lockout
void blink_on_activate(void)
{
    if (shutdown_detected) // do not blink,  power usage needs to be very stable to tell if the host has haulted. 
    {
        return;
    }
    
    unsigned long kRuntime = millis() - blink_started_at;
    
    if (!error_status) 
    {
        // blink half as fast when host is foreign
        if (host_is_foreign)
        {
            kRuntime = kRuntime >> 1;
        }
        
        if ( bootloader_started  && (kRuntime > BLINK_BOOTLD_DELAY) )
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_BOOTLD_DELAY; 
        }
        else if ( lockout_active  && (kRuntime > BLINK_LOCKOUT_DELAY) )
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
            connect_normal_mode();
            host_active =1;
            bootloader_started = 0;
        }
    }
}

void check_DTR(void)
{
    if (!host_is_foreign)
    {
        if ( !digitalRead(HOST_nDTR) )  // both HOST_nDTR and HOST_nRTS are set (active low) when avrdude tries to use the bootloader
        { 
            if ( !(bootloader_started  || lockout_active || host_active || uart_has_TTL) )
            {
                // send a byte on the DTR pair when FTDI_nDTR is first active
                uart_started_at = millis();
                uart_output= bootloader_address; // set by I2C, default is RPU_HOST_CONNECT
                printf("%c", uart_output); 
                uart_has_TTL = 1;
                localhost_active = 1;
            }
        }
        else
        {
            if ( host_active && localhost_active && (!uart_has_TTL) && (!bootloader_started) && (!lockout_active) )
            {
                // send a byte on the DTR pair when FTDI_nDTR is first non-active
                uart_started_at = millis();
                uart_output= RPU_HOST_DISCONNECT;
                printf("%c", uart_output); 
                uart_has_TTL = 1;
                digitalWrite(LED_BUILTIN, HIGH);
                localhost_active = 0;
            }
        }
    }
}

// lockout needs to happoen for a long enough time to insure bootloading is finished,
void check_lockout(void)
{
    unsigned long kRuntime = millis() - lockout_started_at;
    
    if ( lockout_active && (kRuntime > LOCKOUT_DELAY))
    {
        connect_normal_mode();

        host_active = 1;
        lockout_active =0;
    }
}

/* The RPUadpt UART is connected to the DTR pair which is half duplex, 
     but is self enabling when TX is pulled low.
*/
void check_uart(void)
{
    unsigned long kRuntime = millis() - uart_started_at;
 
    if ( uart0_available() && !(uart_has_TTL && (kRuntime > UART_TTL) ) )
    {
        uint8_t input;
        input = (uint8_t)(getchar());

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
            host_is_foreign = 0;
        }
        else
        {
            if (localhost_active)
            {
                host_is_foreign = 0; // used to connect the host
            }
            else
            {
                host_is_foreign = 1; // used to lockout the host
            }
        }

        if (input == RPU_NORMAL_MODE) // end the lockout or bootloader if it was set.
        { 
            lockout_started_at = millis() - LOCKOUT_DELAY;
            bootloader_started_at = millis() - BOOTLOADER_ACTIVE;
            digitalWrite(LED_BUILTIN, LOW);
            blink_started_at = millis();
        }
        else if (input == rpu_address) // that is my local address
        {
            connect_bootload_mode();

            // start the bootloader
            bootloader_started = 1;
            digitalWrite(nSS, LOW);   // nSS goes through a open collector buffer to nRESET
            _delay_ms(20);  // hold reset low for a short time 
            digitalWrite(nSS, HIGH); // this will release the buffer with open colllector on MCU nRESET.
            local_mcu_is_rpu_aware = 0; // after a reset it may be loaded with new software
            blink_started_at = millis();
            bootloader_started_at = millis();
        }
        else if (input <= 0x7F) // values > 0x80 are for a host disconnect e.g. the bitwise negation of an RPU_ADDRESS
        {  
            lockout_active =1;
            bootloader_started = 0;
            host_active =0;

            connect_lockout_mode();

            lockout_started_at = millis();
            blink_started_at = millis();
        }
        else if (input > 0x7F) // RPU_HOST_DISCONNECT is the bitwise negation of an RPU_ADDRESS it will be > 0x80 (seen as a uint8_t)
        { 
            host_is_foreign = 0;
            lockout_active =0;
            host_active =0;
            bootloader_started = 0;
            digitalWrite(LED_BUILTIN, HIGH);
            digitalWrite(RX_DE, LOW); // disallow RX pair driver to enable if FTDI_TX is low
            digitalWrite(RX_nRE, HIGH);  // disable RX pair recevior to output to local MCU's RX input
            digitalWrite(TX_DE, LOW); // disallow TX pair driver to enable if TX (from MCU) is low
            digitalWrite(TX_nRE, HIGH);  // disable TX pair recevior that outputs to FTDI_RX input
        } 
    }
    else if (uart_has_TTL && (kRuntime > UART_TTL) )
    { // perhaps the DTR line is stuck (e.g. someone has pulled it low) so may need to time out
        error_status &= (1<<0);
        uart_has_TTL = 0;
    }
}

void save_rpu_addr_state(void)
{
    if (eeprom_is_ready())
    {
        // up to first EE_RPU_IDMAX states may be used for writhing an ID to the EEPROM
        if ( (write_rpu_address_to_eeprom >= 1) && (write_rpu_address_to_eeprom <= EE_RPU_IDMAX) )
        { // write "RPUadpt\0" at address EE_RPU_ID thru 4A
            uint8_t value = pgm_read_byte(&EE_IdTable[write_rpu_address_to_eeprom-1]);
            eeprom_write_byte( (uint8_t *)((write_rpu_address_to_eeprom-1)+EE_RPU_ID), value);
            
            if (value == '\0') 
            {
                write_rpu_address_to_eeprom = 11;
            }
            else
            {
                write_rpu_address_to_eeprom += 1;
            }
        }
        
        if ( (write_rpu_address_to_eeprom == 11) )
        { // write the rpu address to eeprom address EE_RPU_ADDRESS 
            uint8_t value = rpu_address;
            eeprom_write_byte( (uint8_t *)(EE_RPU_ADDRESS), value);
            write_rpu_address_to_eeprom = 0;
        }
    }
}

void check_shutdown(void)
{
    if (shutdown_started)
    {
        unsigned long kRuntime = millis() - shutdown_started_at;
        
        if ( kRuntime > SHUTDOWN_TIME)
        {
            pinMode(SHUTDOWN, INPUT);
            digitalWrite(SHUTDOWN, HIGH); // trun on a weak pullup 
            shutdown_started = 0; // set with I2C command 5
            shutdown_detected = 1; // clear when reading with I2C command 4
        }
    }
    else
        if (!shutdown_detected)
        { 
            if( !digitalRead(SHUTDOWN) ) 
            {
                pinMode(LED_BUILTIN, OUTPUT);
                digitalWrite(LED_BUILTIN, HIGH);
                shutdown_detected = 1; // clear when reading with I2C command 4
            }
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
        if(write_rpu_address_to_eeprom) save_rpu_addr_state();
        check_shutdown();
    }    
}

