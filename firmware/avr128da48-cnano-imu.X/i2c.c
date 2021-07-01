/*
    \file   i2c.c

    \brief  TWI I2C Driver

    (c) 2020 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third-party
    license terms applicable to your use of third-party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#include "clkctrl.h"
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "i2c.h"

#ifdef TWI0_BAUD
#undef TWI0_BAUD
#endif

#define TWI0_BAUD(F_SCL, T_RISE)        (uint8_t)((((((float)F_CPU / (float)(F_SCL)) - 10 - (((float)(F_CPU) * (T_RISE))/1000000.0))) / 2))

#define I2C_SCL_FREQ                    100000  /* Frequency [Hz] */

enum {
    I2C_INIT = 0, 
    I2C_ACKED,
    I2C_NACKED,
    I2C_READY,
    I2C_ERROR
};

void I2C_0_Init(void)
{
    /* Select I2C pins PC2/PC3 */
    PORTMUX.TWIROUTEA = 0x02;
    
    /* Master Baud Rate Control */
    TWI0.MBAUD = TWI0_BAUD((I2C_SCL_FREQ), 0.3);
    
    /* Enable TWI */ 
    TWI0.MCTRLA = TWI_ENABLE_bm;
    
    /* Initialize the address register */
    TWI0.MADDR = 0x00;
    
    /* Initialize the data register */
    TWI0.MDATA = 0x00;
    
    /* Set bus state idle */
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

static uint8_t i2c_0_WaitW(void)
{
    uint8_t state = I2C_INIT;
    do
    {
        //printf("i2c_0_WaitW - %u \r\n", state);
        if(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))
        {
            if(!(TWI0.MSTATUS & TWI_RXACK_bm))
            {
                /* slave responded with ack - TWI goes to M1 state */
                state = I2C_ACKED;
            }
            else
            {
                /* address sent but no ack received - TWI goes to M3 state */
                state = I2C_NACKED;
            }
        }
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm))
        {
            /* get here only in case of bus error or arbitration lost - M4 state */
            state = I2C_ERROR;
        }
    } while(!state);
    
    return state;
}

static uint8_t i2c_0_WaitR(void)
{
    uint8_t state = I2C_INIT;
    do
    {
         //printf("i2c_0_WaitR - %u \r\n", state);
        if(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm))
        {
            state = I2C_READY;
        }
        else if(TWI0.MSTATUS & (TWI_BUSERR_bm | TWI_ARBLOST_bm))
        {
            /* get here only in case of bus error or arbitration lost - M4 state */
            state = I2C_ERROR;
        }
    } while(!state);
    
    return state;
}

 /* Returns how many bytes have been sent, -1 means NACK at address, 0 means slave ACKed to slave address */
uint8_t I2C_0_SendData(uint8_t address, uint8_t *pData, uint8_t len)
{
    uint8_t retVal = (uint8_t) - 1;
    
    /* start transmitting the slave address */
    TWI0.MADDR = address & ~0x01;
    if(i2c_0_WaitW() != I2C_ACKED)
        return retVal;

    retVal = 0;
    if((len != 0) && (pData != NULL))
    {
        while(len--)
        {
            TWI0.MDATA = *pData;
            if(i2c_0_WaitW() == I2C_ACKED)
            {
                retVal++;
                pData++;
                continue;
            }
            else // did not get ACK after data
            {
                break;
            }
        }
    }
    
    return retVal;
}

/* Returns how many bytes have been received, -1 means NACK at address */
uint8_t I2C_0_GetData(uint8_t address, uint8_t *pData, uint8_t len)
{
    uint8_t retVal = (uint8_t) - 1;
    
    /* start transmitting the slave address */
    TWI0.MADDR = address | 0x01;
    if(i2c_0_WaitW() != I2C_ACKED){
        //printf("\n 4. Did not ack\n");
        //TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
        return retVal;
    }else{
        //printf("\n 4. Did ack\n");
    }

    retVal = 0;
    //printf("\n 5. Before loop -> len: %u \n", len);
    if((len != 0) && (pData !=NULL ))
    {
        while(len--)
        {
            //printf("\n State loop -> len: %u retval: %u \n", len, retVal);
            if(i2c_0_WaitR() == I2C_READY)
            {
               *pData = TWI0.MDATA;
               if(len != 0){
                   TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
               } else {
                   //TWI0.MCTRLB |= 0b11111100; 
                   // FLUSH and NACK needed. Not enough with just NACK.
                   TWI0.MCTRLB = TWI_FLUSH_bm | TWI_ACKACT_NACK_gc;  
               }

                retVal++;
                pData++;
                continue;
            }
            else{
                printf("Error on Wait");
                break;
            }
        
        }
    }
    //printf("\n\r return Value %u", retVal);
    return retVal;
}

void I2C_0_EndSession(void)
{
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
}