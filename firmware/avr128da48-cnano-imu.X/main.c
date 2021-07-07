/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "mcc_generated_files/mcc.h"                // SYS function prototypes
#include "buffer.h"
#include "sensor.h"
#include "app_config.h"
#if STREAM_FORMAT_IS(SMLSS)
#include "ssi_comms.h"
#endif //STREAM_FORMAT_IS(SMLSS)

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

#if STREAM_FORMAT_IS(SMLSS)
static ssi_io_funcs_t ssi_io_s;
#endif //STREAM_FORMAT_IS(SMLSS)

static volatile uint32_t tickcounter = 0;
static volatile unsigned int tickrate = 0;

static struct sensor_device_t sensor;
static struct sensor_buffer_t snsr_buffer;

void Ticker_Callback(void) {
    static unsigned int mstick = 0;

    ++tickcounter;
    if (tickrate == 0) {
        mstick = 0;
    }
    else if (++mstick == tickrate) {
        LED_GREEN_Toggle();
        mstick = 0;
    }
}

uint64_t read_timer_ms(void) {
    return tickcounter;
}

uint64_t read_timer_us(void) {
    return tickcounter * 1000U + (uint32_t) TC_TimerGet();
}

void sleep_ms(uint32_t ms) {
    uint32_t t0 = read_timer_ms();
    while ((read_timer_ms() - t0) < ms) { };
}

void sleep_us(uint32_t us) {
    uint32_t t0 = read_timer_us();
    while ((read_timer_us() - t0) < us) { };
}

// For handling read of the sensor data
void SNSR_ISR_HANDLER(void) {
    /* Check if any errors we've flagged have been acknowledged */
    if (sensor.status != SNSR_STATUS_OK) {
        return;
    }
    
    sensor.status = sensor_read(&sensor, &snsr_buffer);
}

size_t UART_Read(uint8_t *ptr, const size_t nbytes) {
    size_t bytecnt;
    for (bytecnt=0; bytecnt < nbytes; bytecnt++) {
        *(ptr + bytecnt) = USART1_Read();
    }
    return bytecnt;
}

size_t UART_Write(uint8_t *ptr, const size_t nbytes) {
    size_t bytecnt;
    for (bytecnt=0; bytecnt < nbytes; bytecnt++) {
        USART1_Write(*(ptr + bytecnt));
    }
    return bytecnt;
}

#if STREAM_FORMAT_IS(SMLSS)
#define PACKET_BUFFER_BYTE_LEN (SNSR_NUM_AXES * SNSR_SAMPLES_PER_PACKET * sizeof(SNSR_DATA_TYPE))
#define SML_MAX_CFG_STR_SIZE 256
static SNSR_DATA_TYPE packet_data_buffer[SNSR_NUM_AXES * SNSR_SAMPLES_PER_PACKET];
static int num_packets = 0;
static char json_config_str[SML_MAX_CFG_STR_SIZE];

static void connect_reconnect()
{
    uint32_t time = read_timer_ms();
    char newline[] = "\n";

    UART_Write((uint8_t *) json_config_str, strlen(json_config_str));
    UART_Write((uint8_t *) newline, 1);
    while(!ssi_io_s.connected) {
        if (USART1_IsRxReady())
        {
            ssi_try_connect();
        }
        if ((read_timer_ms() - time) >= 1000) {
            time = read_timer_ms();
            //printf("%s\r\n", json_config_str);
            UART_Write((uint8_t *) json_config_str, strlen(json_config_str));
            UART_Write((uint8_t *) newline, 1);
        }
    }
    buffer_reset(&snsr_buffer);
}

static void build_json_config(void)
{
    size_t written=0;
    size_t snsr_index = 0;

    written += snprintf(json_config_str, SML_MAX_CFG_STR_SIZE, "{\"version\":%d, \"sample_rate\":%d,"
                        "\"samples_per_packet\":%d,"
                        "\"column_location\":{"
                        , SSI_JSON_CONFIG_VERSION, SNSR_SAMPLE_RATE, SNSR_SAMPLES_PER_PACKET);
    #if SNSR_USE_ACCEL_X
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"AccelerometerX\":%d,", snsr_index++);
    #endif
    #if SNSR_USE_ACCEL_Y
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"AccelerometerY\":%d,", snsr_index++);
    #endif
    #if SNSR_USE_ACCEL_Z
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"AccelerometerZ\":%d,", snsr_index++);
    #endif
    #if SNSR_USE_GYRO_X
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"GyroscopeX\":%d,", snsr_index++);
    #endif
    #if SNSR_USE_GYRO_Y
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"GyroscopeY\":%d,", snsr_index++);
    #endif
    #if SNSR_USE_GYRO_Z
    written += snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "\"GyroscopeZ\":%d", snsr_index++);
    #endif
    if(json_config_str[written-1] == ',')
    {
        written--;
    }
    snprintf(json_config_str+written, SML_MAX_CFG_STR_SIZE-written, "}}");
}

#endif //STREAM_FORMAT_IS(SMLSS)

int main ( void )
{
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    /* Register and start the LED ticker */
    TC_TimerCallbackRegister(Ticker_Callback);
    TC_TimerStart();
    
    /* Activate External Interrupt Controller for sensor capture */
    MIKRO_INT_CallbackRegister(SNSR_ISR_HANDLER);
    
    /* Initialize our data buffer */
    buffer_init(&snsr_buffer);
    
    printf("\n");
    
    while (1)
    {    
        if (sensor_init(&sensor) != SNSR_STATUS_OK) {
            printf("sensor init result = %d\n", sensor.status);
            break;
        }
        
        if (sensor_set_config(&sensor) != SNSR_STATUS_OK) {
            printf("sensor configuration result = %d\n", sensor.status);
            break;
        }
        
        printf("sensor type is %s\n", SNSR_NAME);
        printf("sensor sample rate set at %d%s\n", SNSR_SAMPLE_RATE, SNSR_SAMPLE_RATE_UNIT_STR);
#if SNSR_USE_ACCEL
        printf("accelerometer axes %s%s%s enabled with range set at +/-%dGs\n", SNSR_USE_ACCEL_X ? "x" : "", SNSR_USE_ACCEL_Y ? "y" : "", SNSR_USE_ACCEL_Z ? "z" : "", SNSR_ACCEL_RANGE);
#else
        printf("accelerometer disabled\n");
#endif
#if SNSR_USE_GYRO
        printf("gyrometer axes %s%s%s enabled with range set at %dDPS\n", SNSR_USE_GYRO_X ? "x" : "", SNSR_USE_GYRO_Y ? "y" : "", SNSR_USE_GYRO_Z ? "z" : "", SNSR_GYRO_RANGE);
#else
        printf("gyrometer disabled\n");
#endif

#if STREAM_FORMAT_IS(SMLSS)
        ssi_io_s.ssi_read = UART_Read;
        ssi_io_s.ssi_write = UART_Write;
        ssi_io_s.connected = false;
        ssi_init(&ssi_io_s);
        build_json_config();
        connect_reconnect();
        
#endif //STREAM_FORMAT_IS(SMLSS)
        buffer_reset(&snsr_buffer);
        
        tickrate = TICK_RATE_SLOW;
        
        break;
    }
    
    while (1)
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        if (sensor.status != SNSR_STATUS_OK) {
            printf("Got a bad sensor status: %d\n", sensor.status);
            break;
        }
        else if (snsr_buffer.overrun == true) {
            printf("\n\n\nOverrun!\n\n\n");
            
            // Light the LEDs to indicate overflow
            tickrate = 0;
            LED_ALL_Off();
            LED_YELLOW_On(); LED_RED_On();  // Indicate OVERFLOW
            sleep_ms(5000U);
            LED_ALL_Off(); // Clear OVERFLOW
            
            buffer_reset(&snsr_buffer); 
            continue;
        }     
        else {
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
                for (int j=1; j < SNSR_NUM_AXES; j++) {
                    printf(" %d", ptr[j]);
                }
                printf("\n");
#elif STREAM_FORMAT_IS(SMLSS)
                for(int i = 0; i < SNSR_NUM_AXES; i++)
                {
                    packet_data_buffer[i+(num_packets * SNSR_NUM_AXES)]=ptr[i];
                }
                num_packets++;
                if(num_packets == SNSR_SAMPLES_PER_PACKET)
                {
                    if(ssi_io_s.connected)
                    {
#                   if SSI_JSON_CONFIG_VERSION==2
                        ssiv2_publish_sensor_data(0, (uint8_t*) packet_data_buffer, PACKET_BUFFER_BYTE_LEN);
#                   elif SSI_JSON_CONFIG_VERSION==1
                        ssiv1_publish_sensor_data((uint8_t*) packet_data_buffer, PACKET_BUFFER_BYTE_LEN);
#                   endif //SSI_JSON_CONFIG_VERSION
                        num_packets = 0;
                    }
                }                
#endif //STREAM_FORMAT_IS
                ptr += SNSR_NUM_AXES;
                buffer_advance_read_index(&snsr_buffer, 1);
            }
        }
        
    }
        
    tickrate = 0;
    LED_GREEN_Off();
    LED_RED_On();
    
    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

