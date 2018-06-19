/*
AVR asynchronous interrupt-driven TWI/I2C C library 
Copyright (C) 2018 Ronald Sutherland

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

Original 2006 Nicholas Zambetti 
Modified in 2012 by Todd Krein (todd@krein.org) to implement repeated starts
Modified in 2016 by Ronald Sutherland (ronald.sutherlad@gmail) to use as a C library with avr-libc dependency
*/

#include <stdbool.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include "twi0.h"

static volatile uint8_t twi0_state;
static volatile uint8_t twi0_slarw;
static volatile uint8_t twi0_sendStop;			// should the transaction end with a stop
static volatile uint8_t twi0_inRepStart;			// in the middle of a repeated start

static void (*twi0_onSlaveTransmit)(void);
static void (*twi0_onSlaveReceive)(uint8_t*, int);

static uint8_t twi0_masterBuffer[TWI0_BUFFER_LENGTH];
static volatile uint8_t twi0_masterBufferIndex;
static volatile uint8_t twi0_masterBufferLength;

static uint8_t twi0_txBuffer[TWI0_BUFFER_LENGTH];
static volatile uint8_t twi0_txBufferIndex;
static volatile uint8_t twi0_txBufferLength;

static uint8_t twi0_rxBuffer[TWI0_BUFFER_LENGTH];
static volatile uint8_t twi0_rxBufferIndex;

static volatile uint8_t twi0_error;

/* init twi pins and set bitrate */
void twi0_init(uint8_t pull_up)
{
    // initialize state
    twi0_state = TWI0_READY;
    twi0_sendStop = 1;		// default value
    twi0_inRepStart = 0;

    // Do not use pull-up for twi pins if the MCU is running at a higher voltage.
    // e.g. if MCU has 5V and others have 3.3V do not use the pull-up. 
    if (pull_up) 
    {
#if defined(__AVR_ATmega328PB__) 
        DDRC &= ~(1 << DDC4);  // clear the ddr bit to set as an input
        PORTC |= (1 << PORTC4);  // write a one to the port bit to enable the pull-up
        DDRC &= ~(1 << DDC5);
        PORTC |= (1 << PORTC5); 
#else
#error "no I2C definition for MCU available"
#endif
    }

    // initialize twi prescaler and bit rate
    TWSR0 &= ~((1<<TWPS0));
    TWSR0 &= ~((1<<TWPS1));
    TWBR0 = ((F_CPU / TWI0_FREQ) - 16) / 2;

    /* twi bit rate formula from atmega128 manual pg 204
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR0))
    note: TWBR0 should be 10 or higher for master mode
    It is 72 for a 16mhz Wiring board with 100kHz TWI */

    // enable twi module, acks, and twi interrupt
    TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
}

/* disables twi pins */
void twi0_disable(void)
{
    // disable twi module, acks, and twi interrupt
    TWCR0 &= ~((1<<TWEN) | (1<<TWIE) | (1<<TWEA));

    // deactivate internal pullups for twi.
#if defined(__AVR_ATmega328PB__) 
    PORTC &= ~(1 << PORTC4);  // clear the port bit to disable the pull-up
    PORTC &= ~(1 << PORTC5); 
#else
#error "no I2C definition for MCU available"
#endif
}

/* init slave address and enable interrupt */
void twi0_setAddress(uint8_t address)
{
    // set twi slave address (skip over TWGCE bit)
    TWAR0 = address << 1;
}

/* set twi bit rate */
void twi0_setFrequency(uint32_t frequency)
{
    TWBR0 = ((F_CPU / frequency) - 16) / 2;

    /* twi bit rate formula from atmega128 manual pg 204
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR0))
    note: TWBR0 should be 10 or higher for master mode
    It is 72 for a 16mhz Wiring board with 100kHz TWI */
}

/* attempt to become twi master and read bytes from a device
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
uint8_t twi0_readFrom(uint8_t address, uint8_t* data, uint8_t length, uint8_t sendStop)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI0_BUFFER_LENGTH < length)
    {
        return 0;
    }

    // wait until twi is ready, become master receiver
    while(TWI0_READY != twi0_state)
    {
        continue;
    }
    twi0_state = TWI0_MRX;
    twi0_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi0_error = 0xFF;

    // initialize buffer iteration vars
    twi0_masterBufferIndex = 0;
    twi0_masterBufferLength = length-1;  // This is not intuitive, read on...
    // On receive, the previously configured ACK/NACK setting is transmitted in
    // response to the received byte before the interrupt is signalled. 
    // Therefor we must actually set NACK when the _next_ to last byte is
    // received, causing that NACK to be sent in response to receiving the last
    // expected byte of data.

    // build sla+w, slave device address + w bit
    twi0_slarw = TW_READ;
    twi0_slarw |= address << 1;

    if (true == twi0_inRepStart) 
    {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the 
        // repeated start that we sent ourselves, and that would really confuse things.
        twi0_inRepStart = false;			// remember, we're dealing with an ASYNC ISR
        do 
        {
            TWDR0 = twi0_slarw;
        } while(TWCR0 & (1<<TWWC));
        TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
    }
    else
    {
        // send start condition
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTA);
    }
        
    // wait for read operation to complete
    while(TWI0_MRX == twi0_state)
    {
        continue;
    }

    if (twi0_masterBufferIndex < length)
    {
        length = twi0_masterBufferIndex;
    }

    // copy twi buffer to data
    for(i = 0; i < length; ++i)
    {
        data[i] = twi0_masterBuffer[i];
    }
	
    return length;
}

/* attempt to become twi bus master and write bytes to a device
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the end
 * Output   0 .. success
 *          1 .. length to long for buffer
 *          2 .. address send, NACK received
 *          3 .. data send, NACK received
 *          4 .. other twi error (lost bus arbitration, bus error, ..)
 */
uint8_t twi0_writeTo(uint8_t address, uint8_t* data, uint8_t length, uint8_t wait, uint8_t sendStop)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI0_BUFFER_LENGTH < length)
    {
        return 1;
    }

    // wait until twi is ready, become master transmitter
    while(TWI0_READY != twi0_state)
    {
        continue;
    }
    twi0_state = TWI0_MTX;
    twi0_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi0_error = 0xFF;

    // initialize buffer iteration vars
    twi0_masterBufferIndex = 0;
    twi0_masterBufferLength = length;
  
    // copy data to twi buffer
    for(i = 0; i < length; ++i)
    {
        twi0_masterBuffer[i] = data[i];
    }
  
    // build sla+w, slave device address + w bit
    twi0_slarw = TW_WRITE; // from #include <util/twi.h> 
    twi0_slarw |= address << 1;
  
    // if we're in a repeated start, then we've already sent the START
    // in the ISR. Don't do it again.
    //
    if (true == twi0_inRepStart) {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the 
        // repeated start that we sent outselves, and that would really confuse things.
        twi0_inRepStart = false;			// remember, we're dealing with an ASYNC ISR
        do 
        {
            TWDR0 = twi0_slarw;				
        } while(TWCR0 & (1<<TWWC));
        TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
    }
    else
    {
        // send start condition
        TWCR0 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);	// enable INTs
    }

    // wait for write operation to complete
    while(wait && (TWI0_MTX == twi0_state))
    {
        continue;
    }
  
    if (twi0_error == 0xFF)
        return 0;	// success
    else if (twi0_error == TW_MT_SLA_NACK)
        return 2;	// error: address send, nack received
    else if (twi0_error == TW_MT_DATA_NACK)
        return 3;	// error: data send, nack received
    else
        return 4;	// other twi error
}

/*  fill slave tx buffer with data that will be used by slave tx event callback
 * Input    data: pointer to byte array
 *          length: number of bytes in array
 * Output   1 length too long for buffer
 *          2 not slave transmitter
 *          0 ok
 */
uint8_t twi0_transmit(const uint8_t* data, uint8_t length)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI0_BUFFER_LENGTH < length)
    {
        return 1;
    }
  
    // ensure we are currently a slave transmitter
    if(TWI0_STX != twi0_state)
    {
        return 2;
    }
  
    // set length and copy data into tx buffer
    twi0_txBufferLength = length;
    for(i = 0; i < length; ++i)
    {
        twi0_txBuffer[i] = data[i];
    }
  
    return 0;
}

/* set function called durring a slave read operation
 * Input    function: callback function to use
 */
void twi0_attachSlaveRxEvent( void (*function)(uint8_t*, int) )
{
    twi0_onSlaveReceive = function;
}

/* sets function called before a slave write operation
 * Input    function: callback function to use
 */
void twi0_attachSlaveTxEvent( void (*function)(void) )
{
    twi0_onSlaveTransmit = function;
}

/* send byte or ready to receive signal
 * Input    ack: byte indicating to ack or to nack
 */
void twi0_reply(uint8_t ack)
{
    // transmit master read ready signal, with or without ack
    if(ack)
    {
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT) | (1<<TWEA);
    }
    else
    {
        TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT);
    }
}

/* relinquishe bus master status */
void twi0_stop(void)
{
    // send stop condition
    TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTO);

    // wait for stop condition to be exectued on bus
    // TWINT is not set after a stop condition!
    while(TWCR0 & (1<<TWSTO))
    {
        continue;
    }

    // update twi state
    twi0_state = TWI0_READY;
}

/* release bus */
void twi0_releaseBus(void)
{
    // release bus
    TWCR0 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT);

    // update twi state
    twi0_state = TWI0_READY;
}

ISR(TWI0_vect)
{
    // #define TW_STATUS   (TWSR & TW_STATUS_MASK)
    switch(TWSR0 & TW_STATUS_MASK)
    {
        // All Master
        case TW_START:     // sent start condition
        case TW_REP_START: // sent repeated start condition
            // copy device address and r/w bit to output register and ack
            TWDR0 = twi0_slarw;
            twi0_reply(1);
            break;

        // Master Transmitter
        case TW_MT_SLA_ACK:  // slave receiver acked address
        case TW_MT_DATA_ACK: // slave receiver acked data
            // if there is data to send, send it, otherwise stop 
            if(twi0_masterBufferIndex < twi0_masterBufferLength)
            {
                // copy data to output register and ack
                TWDR0 = twi0_masterBuffer[twi0_masterBufferIndex++];
                twi0_reply(1);
            }
            else
            {
                if (twi0_sendStop)
                    twi0_stop();
                else 
                {
                    twi0_inRepStart = true;	// we're gonna send the START
                    // don't enable the interrupt. We'll generate the start, but we 
                    // avoid handling the interrupt until we're in the next transaction,
                    // at the point where we would normally issue the start.
                    TWCR0 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                    twi0_state = TWI0_READY;
                }
            }
            break;
            
        case TW_MT_SLA_NACK:  // address sent, nack received
            twi0_error = TW_MT_SLA_NACK;
            twi0_stop();
            break;
        
        case TW_MT_DATA_NACK: // data sent, nack received
            twi0_error = TW_MT_DATA_NACK;
            twi0_stop();
            break;
        
        case TW_MT_ARB_LOST: // lost bus arbitration
            twi0_error = TW_MT_ARB_LOST;
            twi0_releaseBus();
            break;

        // Master Receiver
        case TW_MR_DATA_ACK: // data received, ack sent
            // put byte into buffer
            twi0_masterBuffer[twi0_masterBufferIndex++] = TWDR0;
        case TW_MR_SLA_ACK:  // address sent, ack received
            // ack if more bytes are expected, otherwise nack
            if(twi0_masterBufferIndex < twi0_masterBufferLength)
            {
                twi0_reply(1);
            }
            else
            {
                twi0_reply(0);
            }
            break;
            
        case TW_MR_DATA_NACK: // data received, nack sent
            // put final byte into buffer
            twi0_masterBuffer[twi0_masterBufferIndex++] = TWDR0;
            if (twi0_sendStop)
                twi0_stop();
            else 
            {
                twi0_inRepStart = true;	// we're gonna send the START
                // don't enable the interrupt. We'll generate the start, but we 
                // avoid handling the interrupt until we're in the next transaction,
                // at the point where we would normally issue the start.
                TWCR0 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                twi0_state = TWI0_READY;
            }    
            break;
            
        case TW_MR_SLA_NACK: // address sent, nack received
            twi0_stop();
            break;
        
        // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

        // Slave Receiver
        case TW_SR_SLA_ACK:   // addressed, returned ack
        case TW_SR_GCALL_ACK: // addressed generally, returned ack
        case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
        case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
            // enter slave receiver mode
            twi0_state = TWI0_SRX;
            // indicate that rx buffer can be overwritten and ack
            twi0_rxBufferIndex = 0;
            twi0_reply(1);
            break;
        
        case TW_SR_DATA_ACK:       // data received, returned ack
        case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
            // if there is still room in the rx buffer
            if(twi0_rxBufferIndex < TWI0_BUFFER_LENGTH)
            {
                // put byte in buffer and ack
                twi0_rxBuffer[twi0_rxBufferIndex++] = TWDR0;
                twi0_reply(1);
            }
            else
            {
                // otherwise nack
                twi0_reply(0);
            }
            break;

        case TW_SR_STOP: // stop or repeated start condition received
            // ack future responses and leave slave receiver state
            twi0_releaseBus();
            // put a null char after data if there's room
            if(twi0_rxBufferIndex < TWI0_BUFFER_LENGTH)
            {
                twi0_rxBuffer[twi0_rxBufferIndex] = '\0';
            }
            // callback to user defined callback
            twi0_onSlaveReceive(twi0_rxBuffer, twi0_rxBufferIndex);
            // since we submit rx buffer to "wire" library, we can reset it
            twi0_rxBufferIndex = 0;
            break;

        case TW_SR_DATA_NACK:       // data received, returned nack
        case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
            // nack back at master
            twi0_reply(0);
            break;
        
        // Slave Transmitter
        case TW_ST_SLA_ACK:          // addressed, returned ack
        case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
            // enter slave transmitter mode
            twi0_state = TWI0_STX;
            // ready the tx buffer index for iteration
            twi0_txBufferIndex = 0;
            // set tx buffer length to be zero, to verify if user changes it
            twi0_txBufferLength = 0;
            // request for txBuffer to be filled and length to be set
            // note: user must call twi0_transmit(bytes, length) to do this
            twi0_onSlaveTransmit();
            // if they didn't change buffer & length, initialize it
            if(0 == twi0_txBufferLength)
            {
                twi0_txBufferLength = 1;
                twi0_txBuffer[0] = 0x00;
            }
            // transmit first byte from buffer, fall
        case TW_ST_DATA_ACK: // byte sent, ack returned
            // copy data to output register
            TWDR0 = twi0_txBuffer[twi0_txBufferIndex++];
            // if there is more to send, ack, otherwise nack
            if(twi0_txBufferIndex < twi0_txBufferLength)
            {
                twi0_reply(1);
            }
            else
            {
                twi0_reply(0);
            }
            break;
            
        case TW_ST_DATA_NACK: // received nack, we are done 
        case TW_ST_LAST_DATA: // received ack, but we are done already!
            // ack future responses
            twi0_reply(1);
            // leave slave receiver state
            twi0_state = TWI0_READY;
            break;

        // All
        case TW_NO_INFO:   // no state information
            break;
        
        case TW_BUS_ERROR: // bus error, illegal stop/start
            twi0_error = TW_BUS_ERROR;
            twi0_stop();
            break;
    }
}

