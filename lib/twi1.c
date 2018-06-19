/*
AVR asynchronous interrupt-driven TWI/I2C C library 
Copyright (C) 2016 Ronald Sutherland

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

#include "twi1.h"

static volatile uint8_t twi1_state;
static volatile uint8_t twi1_slarw;
static volatile uint8_t twi1_sendStop;			// should the transaction end with a stop
static volatile uint8_t twi1_inRepStart;			// in the middle of a repeated start

static void (*twi1_onSlaveTransmit)(void);
static void (*twi1_onSlaveReceive)(uint8_t*, int);

static uint8_t twi1_masterBuffer[TWI1_BUFFER_LENGTH];
static volatile uint8_t twi1_masterBufferIndex;
static volatile uint8_t twi1_masterBufferLength;

static uint8_t twi1_txBuffer[TWI1_BUFFER_LENGTH];
static volatile uint8_t twi1_txBufferIndex;
static volatile uint8_t twi1_txBufferLength;

static uint8_t twi1_rxBuffer[TWI1_BUFFER_LENGTH];
static volatile uint8_t twi1_rxBufferIndex;

static volatile uint8_t twi1_error;

/* init twi pins and set bitrate */
void twi1_init(uint8_t pull_up)
{
    // initialize state
    twi1_state = TWI1_READY;
    twi1_sendStop = 1;		// default value
    twi1_inRepStart = 0;

    // Do not use pull-up for twi pins if the MCU is running at a higher voltage.
    // e.g. if MCU has 5V and others have 3.3V do not use the pull-up. 
    if (pull_up) 
    {
#if defined(__AVR_ATmega328PB__) 
        DDRE &= ~(1 << DDE0);  // clear the ddr bit to set as an input
        PORTE |= (1 << PORTE0);  // write a one to the port bit to enable the pull-up
        DDRE &= ~(1 << DDE1);
        PORTE |= (1 << PORTE1); 
#else
#error "no I2C definition for MCU available"
#endif
    }

    // initialize twi prescaler and bit rate
    TWSR1 &= ~((1<<TWPS0));
    TWSR1 &= ~((1<<TWPS1));
    TWBR1 = ((F_CPU / TWI1_FREQ) - 16) / 2;

    /* twi bit rate formula from atmega128 manual pg 204
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR1))
    note: TWBR1 should be 10 or higher for master mode
    It is 72 for a 16mhz Wiring board with 100kHz TWI */

    // enable twi module, acks, and twi interrupt
    TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA);
}

/* disables twi pins */
void twi1_disable(void)
{
    // disable twi module, acks, and twi interrupt
    TWCR1 &= ~((1<<TWEN) | (1<<TWIE) | (1<<TWEA));

    // deactivate internal pullups for twi.
#if defined(__AVR_ATmega328PB__) 
    PORTE &= ~(1 << PORTE0);  // clear the port bit to disable the pull-up
    PORTE &= ~(1 << PORTE1); 
#else
#error "no I2C definition for MCU available"
#endif
}

/* init slave address and enable interrupt */
void twi1_setAddress(uint8_t address)
{
    // set twi slave address (skip over TWGCE bit)
    TWAR1 = address << 1;
}

/* set twi bit rate */
void twi1_setFrequency(uint32_t frequency)
{
    TWBR1 = ((F_CPU / frequency) - 16) / 2;

    /* twi bit rate formula from atmega128 manual pg 204
    SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR1))
    note: TWBR1 should be 10 or higher for master mode
    It is 72 for a 16mhz Wiring board with 100kHz TWI */
}

/* attempt to become twi master and read bytes from a device
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
uint8_t twi1_readFrom(uint8_t address, uint8_t* data, uint8_t length, uint8_t sendStop)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI1_BUFFER_LENGTH < length)
    {
        return 0;
    }

    // wait until twi is ready, become master receiver
    while(TWI1_READY != twi1_state)
    {
        continue;
    }
    twi1_state = TWI1_MRX;
    twi1_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi1_error = 0xFF;

    // initialize buffer iteration vars
    twi1_masterBufferIndex = 0;
    twi1_masterBufferLength = length-1;  // This is not intuitive, read on...
    // On receive, the previously configured ACK/NACK setting is transmitted in
    // response to the received byte before the interrupt is signalled. 
    // Therefor we must actually set NACK when the _next_ to last byte is
    // received, causing that NACK to be sent in response to receiving the last
    // expected byte of data.

    // build sla+w, slave device address + w bit
    twi1_slarw = TW_READ;
    twi1_slarw |= address << 1;

    if (true == twi1_inRepStart) 
    {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the 
        // repeated start that we sent ourselves, and that would really confuse things.
        twi1_inRepStart = false;			// remember, we're dealing with an ASYNC ISR
        do 
        {
            TWDR1 = twi1_slarw;
        } while(TWCR1 & (1<<TWWC));
        TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
    }
    else
    {
        // send start condition
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTA);
    }
        
    // wait for read operation to complete
    while(TWI1_MRX == twi1_state)
    {
        continue;
    }

    if (twi1_masterBufferIndex < length)
    {
        length = twi1_masterBufferIndex;
    }

    // copy twi buffer to data
    for(i = 0; i < length; ++i)
    {
        data[i] = twi1_masterBuffer[i];
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
uint8_t twi1_writeTo(uint8_t address, uint8_t* data, uint8_t length, uint8_t wait, uint8_t sendStop)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI1_BUFFER_LENGTH < length)
    {
        return 1;
    }

    // wait until twi is ready, become master transmitter
    while(TWI1_READY != twi1_state)
    {
        continue;
    }
    twi1_state = TWI1_MTX;
    twi1_sendStop = sendStop;
    // reset error state (0xFF.. no error occured)
    twi1_error = 0xFF;

    // initialize buffer iteration vars
    twi1_masterBufferIndex = 0;
    twi1_masterBufferLength = length;
  
    // copy data to twi buffer
    for(i = 0; i < length; ++i)
    {
        twi1_masterBuffer[i] = data[i];
    }
  
    // build sla+w, slave device address + w bit
    twi1_slarw = TW_WRITE; // from #include <util/twi.h> 
    twi1_slarw |= address << 1;
  
    // if we're in a repeated start, then we've already sent the START
    // in the ISR. Don't do it again.
    //
    if (true == twi1_inRepStart) {
        // if we're in the repeated start state, then we've already sent the start,
        // (@@@ we hope), and the TWI statemachine is just waiting for the address byte.
        // We need to remove ourselves from the repeated start state before we enable interrupts,
        // since the ISR is ASYNC, and we could get confused if we hit the ISR before cleaning
        // up. Also, don't enable the START interrupt. There may be one pending from the 
        // repeated start that we sent outselves, and that would really confuse things.
        twi1_inRepStart = false;			// remember, we're dealing with an ASYNC ISR
        do 
        {
            TWDR1 = twi1_slarw;				
        } while(TWCR1 & (1<<TWWC));
        TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE);	// enable INTs, but not START
    }
    else
    {
        // send start condition
        TWCR1 = (1<<TWINT) | (1<<TWEA) | (1<<TWEN) | (1<<TWIE) | (1<<TWSTA);	// enable INTs
    }

    // wait for write operation to complete
    while(wait && (TWI1_MTX == twi1_state))
    {
        continue;
    }
  
    if (twi1_error == 0xFF)
        return 0;	// success
    else if (twi1_error == TW_MT_SLA_NACK)
        return 2;	// error: address send, nack received
    else if (twi1_error == TW_MT_DATA_NACK)
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
uint8_t twi1_transmit(const uint8_t* data, uint8_t length)
{
    uint8_t i;

    // ensure data will fit into buffer
    if(TWI1_BUFFER_LENGTH < length)
    {
        return 1;
    }
  
    // ensure we are currently a slave transmitter
    if(TWI1_STX != twi1_state)
    {
        return 2;
    }
  
    // set length and copy data into tx buffer
    twi1_txBufferLength = length;
    for(i = 0; i < length; ++i)
    {
        twi1_txBuffer[i] = data[i];
    }
  
    return 0;
}

/* set function called durring a slave read operation
 * Input    function: callback function to use
 */
void twi1_attachSlaveRxEvent( void (*function)(uint8_t*, int) )
{
    twi1_onSlaveReceive = function;
}

/* sets function called before a slave write operation
 * Input    function: callback function to use
 */
void twi1_attachSlaveTxEvent( void (*function)(void) )
{
    twi1_onSlaveTransmit = function;
}

/* send byte or ready to receive signal
 * Input    ack: byte indicating to ack or to nack
 */
void twi1_reply(uint8_t ack)
{
    // transmit master read ready signal, with or without ack
    if(ack)
    {
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT) | (1<<TWEA);
    }
    else
    {
        TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWINT);
    }
}

/* relinquishe bus master status */
void twi1_stop(void)
{
    // send stop condition
    TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT) | (1<<TWSTO);

    // wait for stop condition to be exectued on bus
    // TWINT is not set after a stop condition!
    while(TWCR1 & (1<<TWSTO))
    {
        continue;
    }

    // update twi state
    twi1_state = TWI1_READY;
}

/* release bus */
void twi1_releaseBus(void)
{
    // release bus
    TWCR1 = (1<<TWEN) | (1<<TWIE) | (1<<TWEA) | (1<<TWINT);

    // update twi state
    twi1_state = TWI1_READY;
}

ISR(TWI1_vect)
{
    // #define TW_STATUS   (TWSR & TW_STATUS_MASK)
    switch(TWSR1 & TW_STATUS_MASK) //
    {
        // All Master
        case TW_START:     // sent start condition
        case TW_REP_START: // sent repeated start condition
            // copy device address and r/w bit to output register and ack
            TWDR1 = twi1_slarw;
            twi1_reply(1);
            break;

        // Master Transmitter
        case TW_MT_SLA_ACK:  // slave receiver acked address
        case TW_MT_DATA_ACK: // slave receiver acked data
            // if there is data to send, send it, otherwise stop 
            if(twi1_masterBufferIndex < twi1_masterBufferLength)
            {
                // copy data to output register and ack
                TWDR1 = twi1_masterBuffer[twi1_masterBufferIndex++];
                twi1_reply(1);
            }
            else
            {
                if (twi1_sendStop)
                    twi1_stop();
                else 
                {
                    twi1_inRepStart = true;	// we're gonna send the START
                    // don't enable the interrupt. We'll generate the start, but we 
                    // avoid handling the interrupt until we're in the next transaction,
                    // at the point where we would normally issue the start.
                    TWCR1 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                    twi1_state = TWI1_READY;
                }
            }
            break;
            
        case TW_MT_SLA_NACK:  // address sent, nack received
            twi1_error = TW_MT_SLA_NACK;
            twi1_stop();
            break;
        
        case TW_MT_DATA_NACK: // data sent, nack received
            twi1_error = TW_MT_DATA_NACK;
            twi1_stop();
            break;
        
        case TW_MT_ARB_LOST: // lost bus arbitration
            twi1_error = TW_MT_ARB_LOST;
            twi1_releaseBus();
            break;

        // Master Receiver
        case TW_MR_DATA_ACK: // data received, ack sent
            // put byte into buffer
            twi1_masterBuffer[twi1_masterBufferIndex++] = TWDR1;
        case TW_MR_SLA_ACK:  // address sent, ack received
            // ack if more bytes are expected, otherwise nack
            if(twi1_masterBufferIndex < twi1_masterBufferLength)
            {
                twi1_reply(1);
            }
            else
            {
                twi1_reply(0);
            }
            break;
            
        case TW_MR_DATA_NACK: // data received, nack sent
            // put final byte into buffer
            twi1_masterBuffer[twi1_masterBufferIndex++] = TWDR1;
            if (twi1_sendStop)
                twi1_stop();
            else 
            {
                twi1_inRepStart = true;	// we're gonna send the START
                // don't enable the interrupt. We'll generate the start, but we 
                // avoid handling the interrupt until we're in the next transaction,
                // at the point where we would normally issue the start.
                TWCR1 = (1<<TWINT) | (1<<TWSTA)| (1<<TWEN) ;
                twi1_state = TWI1_READY;
            }    
            break;
            
        case TW_MR_SLA_NACK: // address sent, nack received
            twi1_stop();
            break;
        
        // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

        // Slave Receiver
        case TW_SR_SLA_ACK:   // addressed, returned ack
        case TW_SR_GCALL_ACK: // addressed generally, returned ack
        case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
        case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
            // enter slave receiver mode
            twi1_state = TWI1_SRX;
            // indicate that rx buffer can be overwritten and ack
            twi1_rxBufferIndex = 0;
            twi1_reply(1);
            break;
        
        case TW_SR_DATA_ACK:       // data received, returned ack
        case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
            // if there is still room in the rx buffer
            if(twi1_rxBufferIndex < TWI1_BUFFER_LENGTH)
            {
                // put byte in buffer and ack
                twi1_rxBuffer[twi1_rxBufferIndex++] = TWDR1;
                twi1_reply(1);
            }
            else
            {
                // otherwise nack
                twi1_reply(0);
            }
            break;

        case TW_SR_STOP: // stop or repeated start condition received
            // ack future responses and leave slave receiver state
            twi1_releaseBus();
            // put a null char after data if there's room
            if(twi1_rxBufferIndex < TWI1_BUFFER_LENGTH)
            {
                twi1_rxBuffer[twi1_rxBufferIndex] = '\0';
            }
            // callback to user defined callback
            twi1_onSlaveReceive(twi1_rxBuffer, twi1_rxBufferIndex);
            // since we submit rx buffer to "wire" library, we can reset it
            twi1_rxBufferIndex = 0;
            break;

        case TW_SR_DATA_NACK:       // data received, returned nack
        case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
            // nack back at master
            twi1_reply(0);
            break;
        
        // Slave Transmitter
        case TW_ST_SLA_ACK:          // addressed, returned ack
        case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
            // enter slave transmitter mode
            twi1_state = TWI1_STX;
            // ready the tx buffer index for iteration
            twi1_txBufferIndex = 0;
            // set tx buffer length to be zero, to verify if user changes it
            twi1_txBufferLength = 0;
            // request for txBuffer to be filled and length to be set
            // note: user must call twi1_transmit(bytes, length) to do this
            twi1_onSlaveTransmit();
            // if they didn't change buffer & length, initialize it
            if(0 == twi1_txBufferLength)
            {
                twi1_txBufferLength = 1;
                twi1_txBuffer[0] = 0x00;
            }
            // transmit first byte from buffer, fall
        case TW_ST_DATA_ACK: // byte sent, ack returned
            // copy data to output register
            TWDR1 = twi1_txBuffer[twi1_txBufferIndex++];
            // if there is more to send, ack, otherwise nack
            if(twi1_txBufferIndex < twi1_txBufferLength)
            {
                twi1_reply(1);
            }
            else
            {
                twi1_reply(0);
            }
            break;
            
        case TW_ST_DATA_NACK: // received nack, we are done 
        case TW_ST_LAST_DATA: // received ack, but we are done already!
            // ack future responses
            twi1_reply(1);
            // leave slave receiver state
            twi1_state = TWI1_READY;
            break;

        // All
        case TW_NO_INFO:   // no state information
            break;
        
        case TW_BUS_ERROR: // bus error, illegal stop/start
            twi1_error = TW_BUS_ERROR;
            twi1_stop();
            break;
    }
}

