/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
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
#include <avr/interrupt.h>
#include <util/delay.h>

#include "port.h"
#include "usart1.h"
#include "i2c.h"

#include "sensor.h"
#include "buffer.h"
#include "sensor.h"
#include "app_config.h"

static struct sensor_device_t sensor;
static struct sensor_buffer_t snsr_buffer;

ISR(PORTD_PORT_vect)
{
    /* Check if any errors we've flagged have been acknowledged */
    if (sensor.status != SNSR_STATUS_OK) {
        return;
    }
    
    sensor.status = sensor_read(&sensor, &snsr_buffer);
    
    PORTD.INTFLAGS = 0xff;
}

void sleep_ms(uint32_t ms) {
    for(uint32_t i = 0; i < ms; i++) {
        _delay_ms(1);
    }
}

int main(void)
{
    /* Initializes MCU, drivers and middleware */
    CLKCTRL_Init();
    I2C_0_Init();
    USART1_Init();
    PORT_init();
    
    // Initilize sensors
    // enable global interrupt
    sei();
    
    printf("Start\r\n");
    
    while (1) {
        if (sensor_init(&sensor) != SNSR_STATUS_OK) {
            printf("sensor init result = %d\n", sensor.status);
            break;
        }

        if (sensor_set_config(&sensor) != SNSR_STATUS_OK) {
            printf("sensor configuration result = %d\n", sensor.status);
            break;
        }
        break;
    }
    
    /* Replace with your application code */
    while (1){
        if (sensor.status != SNSR_STATUS_OK) 
        {    
            printf("bmi160_get_sensor_data failed...\n");
            break;
        } 
        else 
        {
            buffer_data_t *ptr;
            int rdcnt = buffer_get_read_buffer(&snsr_buffer, &ptr);
            while ( --rdcnt >= 0 ) {
#if STREAM_FORMAT_IS(MDV)
                uint8_t headerbyte = MDV_START_OF_FRAME;
                
                USART1_Write(headerbyte);

                for (int bytecnt=0; bytecnt < sizeof(buffer_frame_t); bytecnt++) {
                    USART1_Write(*((uint8_t *) ptr + bytecnt));
                }
                
                headerbyte = ~headerbyte;
                USART1_Write(headerbyte);
#elif STREAM_FORMAT_IS(ASCII)
                printf("%d", ptr[0]);
                for (int i=1; i < SNSR_NUM_AXES; i++) {
                    printf(" %d", ptr[i]);
                }
                printf("\n");
#endif
                ptr += SNSR_NUM_AXES;
                buffer_advance_read_index(&snsr_buffer, 1);
            }
        }
       
    }
}
/**
    End of File
*/