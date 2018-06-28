/* Remote is for bus manager on RPUadpt, Host2Remote is for RPUftdi
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
#include "../lib/twi0.h"
#include "../lib/twi1.h"
#include "../lib/uart.h"
#include "../lib/pin_num.h"
#include "../lib/pins_board.h"

//I2C_ADDRESS slave address is defined in the Makefile use numbers between 0x08 to 0x78
// RPU_ADDRESS is defined in the Makefile it is used as an ascii characer
// If the RPU_ADDRESS is defined as '1' then the addrss is 0x31. It may then be used as part of a textual command e.g. /1/id?
// RPU_HOST_CONNECT is defined in the Makefile and is used as default address sent on DTR pair if FTDI_nDTR toggles
// byte broadcast on DTR pair when HOST_nDTR (or HOST_nRTS) is no longer active
#define RPU_HOST_DISCONNECT ~RPU_HOST_CONNECT
// return to normal mode address sent on DTR pair
#define RPU_NORMAL_MODE 0x00
// disconnect and set error mode on DTR pair
#define RPU_ERROR_MODE 0xFF

// If this ID is matched in EEPROM then the rpu_address is taken from EEPROM
#define EE_RPU_IDMAX 10

const uint8_t EE_IdTable[] PROGMEM =
{
    'R',
    'P',
    'U',
    'i',
    'd',
    '\0' // null term
};
#define EE_RPU_ID 40
#define EE_RPU_ADDRESS 50

static uint8_t i2c0Buffer[TWI0_BUFFER_LENGTH];
static uint8_t i2c0BufferLength = 0;
static uint8_t i2c1Buffer[TWI1_BUFFER_LENGTH];
static uint8_t i2c1BufferLength = 0;
static uint8_t i2c1_oldBuffer[TWI1_BUFFER_LENGTH]; //i2c1_old is for SMBus
static uint8_t i2c1_oldBufferLength = 0;

#define BOOTLOADER_ACTIVE 115000
#define BLINK_BOOTLD_DELAY 75
#define BLINK_ACTIVE_DELAY 500
#define BLINK_LOCKOUT_DELAY 2000
#define BLINK_STATUS_DELAY 200
#define UART_TTL 500
#define LOCKOUT_DELAY 120000
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

// status_byt bits
#define DTR_READBACK_TIMEOUT 0
#define DTR_I2C_TRANSMIT_FAIL 1
#define DTR_READBACK_NOT_MATCH 2
#define HOST_LOCKOUT_STATUS 3

volatile uint8_t status_byt;
volatile uint8_t uart_output;

// I2C Commands
#define I2C_COMMAND_TO_READ_RPU_ADDRESS 0
#define I2C_COMMAND_TO_SET_RPU_ADDRESS 1
#define I2C_COMMAND_TO_READ_ADDRESS_SENT_ON_ACTIVE_DTR 2
#define I2C_COMMAND_TO_SET_ADDRESS_SENT_ON_ACTIVE_DTR 3
#define I2C_COMMAND_TO_READ_SW_SHUTDOWN_DETECTED 4
#define I2C_COMMAND_TO_SET_SW_FOR_SHUTDOWN 5
#define I2C_COMMAND_TO_READ_STATUS 6
#define I2C_COMMAND_TO_SET_STATUS 7


// called when I2C data is received. 
// RPU Commands on I2C0 does 0..7
void receive0_event(uint8_t* inBytes, int numBytes) 
{
   
    // This buffer will echo's back with transmit0_event()
    for(uint8_t i = 0; i < numBytes; ++i)
    {
        i2c0Buffer[i] = inBytes[i];    
    }
    i2c0BufferLength = numBytes;
    if (i2c0BufferLength > 1)
    {
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_READ_RPU_ADDRESS) )
        {
            i2c0Buffer[1] = rpu_address; // '1' is 0x31
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
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_SET_RPU_ADDRESS) )
        {
            rpu_address = i2c0Buffer[1];
            write_rpu_address_to_eeprom = 1;
        }
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_READ_ADDRESS_SENT_ON_ACTIVE_DTR) )
        {  // read byte sent when HOST_nDTR toggles
            i2c0Buffer[1] = bootloader_address;
        }
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_SET_ADDRESS_SENT_ON_ACTIVE_DTR) ) 
        { // set the byte that is sent when HOST_nDTR toggles
            bootloader_address = i2c0Buffer[1];
        }
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_READ_SW_SHUTDOWN_DETECTED) ) 
        { // when ICP1 pin is pulled  down the host (e.g. Pi Zero on RPUpi) should hault
            i2c0Buffer[1] = shutdown_detected;
             // reading clears this flag that was set in check_shutdown() but it is up to the I2C master to do somthing about it.
            shutdown_detected = 0;
        }
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_SET_SW_FOR_SHUTDOWN) ) 
        { // pull ICP1 pin low to hault the host (e.g. Pi Zero on RPUpi)
            if (i2c0Buffer[1] == 1)
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
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_READ_STATUS) )
        {
            i2c0Buffer[1] = status_byt;
        }
        if ( (i2c0Buffer[0] == I2C_COMMAND_TO_SET_STATUS) )
        {
            status_byt = i2c0Buffer[1];
        }
    }
}

// called when I2C0 data is requested.
void transmit0_event(void) 
{
    // respond with an echo of the last message sent
    uint8_t return_code = twi0_transmit(i2c0Buffer, i2c0BufferLength);
    if (return_code != 0)
        status_byt &= (1<<DTR_I2C_TRANSMIT_FAIL);
}

// called when I2C1 slave has received data
// Host Commands on I2C1 does 0, 2, 3, 6, 7
void receive1_event(uint8_t* inBytes, int numBytes) 
{
    for(uint8_t i = 0; i < i2c1BufferLength; ++i)
    {
        i2c1_oldBuffer[i] = i2c1Buffer[i];    
    }
    i2c1_oldBufferLength = i2c1BufferLength;
    for(uint8_t i = 0; i < numBytes; ++i)
    {
        i2c1Buffer[i] = inBytes[i];    
    }
    i2c1BufferLength = numBytes;
    // skip commands without data and assume they are for read_i2c_block_data
    if (i2c1BufferLength > 1)
    {
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_READ_RPU_ADDRESS) ) // 0
        {
            i2c1Buffer[1] = rpu_address; // '1' is 0x31
            // host reading does not mean the local_mcu_is_rpu_aware
            // also do not change the bus state
            // just tell the host what the local RPU address is
        }
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_READ_ADDRESS_SENT_ON_ACTIVE_DTR) ) // 2
        {  // read byte sent when HOST_nDTR toggles
            i2c1Buffer[1] = bootloader_address;
        }
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_SET_ADDRESS_SENT_ON_ACTIVE_DTR) )  // 3
        { // set the byte that is sent when HOST_nDTR toggles
            bootloader_address = i2c1Buffer[1];
        }
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_READ_STATUS) ) // 6
        {
            i2c1Buffer[1] = status_byt;
        }
        if ( (i2c1Buffer[0] == I2C_COMMAND_TO_SET_STATUS) ) // 7
        {
            status_byt = i2c1Buffer[1];
        }
    }
}

// called when I2C1 slave has been requested to send data
void transmit1_event(void) 
{
    // For SMBus echo the old data from the previous I2C receive event
    twi1_transmit(i2c1_oldBuffer, i2c1_oldBufferLength);
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
    digitalWrite(HOST_nCTS, HIGH);
    pinMode(HOST_nDTR, INPUT);
    digitalWrite(HOST_nDTR, HIGH); // another weak pullup 
    pinMode(HOST_nDSR, OUTPUT);
    digitalWrite(HOST_nDSR, HIGH);
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
    status_byt = 0;
    write_rpu_address_to_eeprom = 0;
    shutdown_detected = 0;
    shutdown_started = 0;

    //Timer0 Fast PWM mode, Timer1 & Timer2 Phase Correct PWM mode.
    initTimers(); 

    /* Initialize UART, it returns a pointer to FILE so redirect of stdin and stdout works*/
    stdout = stdin = uartstream0_init(BAUD);

    twi0_setAddress(I2C0_ADDRESS);
    twi0_attachSlaveTxEvent(transmit0_event); // called when I2C0 slave has been requested to send data
    twi0_attachSlaveRxEvent(receive0_event); // called when I2C0 slave has received data
    twi0_init(false); // do not use internal pull-up

    twi1_setAddress(I2C1_ADDRESS);
    twi1_attachSlaveTxEvent(transmit1_event); // called when I2C1 slave has been requested to send data
    twi1_attachSlaveRxEvent(receive1_event); // called when I2C1 slave has received data
    twi1_init(false); // do not use internal pull-up a Raspberry Pi has them on board

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
    
#if defined(DISCONNECT_AT_PWRUP)
    // at power up send a byte on the DTR pair to unlock the bus 
    // problem is if a foreign host has the bus this would be bad
    uart_started_at = millis();
    uart_output= RPU_HOST_DISCONNECT;
    printf("%c", uart_output); 
#endif
#if defined(HOST_LOCKOUT)
// this will keep the host off the bus until the HOST_LOCKOUT_STATUS bit in status_byt is clear 
// status_byt is zero at this point, but this shows how to set the bit without changing other bits
    status_byt |= (1<<HOST_LOCKOUT_STATUS);
#endif
}

// blink if the host is active, fast blink if status_byt, slow blink in lockout
void blink_on_activate(void)
{
    if (shutdown_detected) // do not blink,  power usage needs to be very stable to tell if the host has haulted. 
    {
        return;
    }
    
    unsigned long kRuntime = millis() - blink_started_at;
    
    // Remote will start with the lockout bit set so don't blink for that
    if (!(status_byt & ~(1<<HOST_LOCKOUT_STATUS) )) 
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
        if ( (kRuntime > BLINK_STATUS_DELAY))
        {
            digitalToggle(LED_BUILTIN);
            
            // next toggle 
            blink_started_at += BLINK_STATUS_DELAY; 
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
        if ( !digitalRead(HOST_nDTR) || !digitalRead(HOST_nRTS) )  // if HOST_nDTR or HOST_nRTS are set (active low) then assume avrdude wants to use the bootloader
        {
            if ( !(status_byt & (1<<HOST_LOCKOUT_STATUS)) )
            {
                if (digitalRead(HOST_nCTS))
                { // tell the host that it is OK to send
                    digitalWrite(HOST_nCTS, LOW);
                    digitalWrite(HOST_nDSR, LOW);
                }
                else
                {
                    if ( !(bootloader_started  || lockout_active || host_active || uart_has_TTL) )
                    {
                        // send the bootload_addres on the DTR pair when nDTR/nRTS becomes active
                        uart_started_at = millis();
                        uart_output= bootloader_address; // set by I2C, default is RPU_HOST_CONNECT
                        printf("%c", uart_output); 
                        uart_has_TTL = 1;
                        localhost_active = 1;
                    }
                }
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
                digitalWrite(HOST_nCTS, HIGH);
                digitalWrite(HOST_nDSR, HIGH);
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

/* The UART is connected to the DTR pair which is half duplex, 
     but is self enabling when TX is pulled low.
*/
void check_uart(void)
{
    unsigned long kRuntime = millis() - uart_started_at;
 
    if ( uart0_available() && !(uart_has_TTL && (kRuntime > UART_TTL) ) )
    {
        uint8_t input;
        input = (uint8_t)(getchar());

        // was this byte sent with the local DTR pair driver, if so the status_byt may need update
        // and the lockout from a local host needs to be treated differently
        // need to ignore the local host's DTR if getting control from a remote host
        if ( uart_has_TTL )
        {
            if(input != uart_output) 
            { // sent byte does not match,  but I'm not to sure what would cause this.
                status_byt &= (1<<DTR_READBACK_NOT_MATCH);
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
        status_byt &= (1<<DTR_READBACK_TIMEOUT);
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

