![https://www.microchip.com/](assets/microchip.png)
# AVR128DA48 Curiosity Nano Data Logger
## Repository Overview
This repository contains firmware for streaming up to 6-axes IMU data over UART from the AVR128DA48 Curiosity Nano board with Base board and Mikroe IMU2 Click board, streaming using one of several formats as described in the sections below.

## Hardware Used
* AVR128DA48 Curiosity Nano Evaluation Kit [(DM164151)](https://www.microchip.com/Developmenttools/ProductDetails/DM164151)
* Curiosity Nano Base for Click boards™ [(AC164162)](https://www.microchip.com/developmenttools/ProductDetails/AC164162)
* IMU 2 click board (https://www.mikroe.com/6dof-imu-2-click)

## Software Used
* MPLAB® X IDE (https://microchip.com/mplab/mplab-x-ide)
* MPLAB® XC8 compiler (https://microchip.com/mplab/compilers)
* MPLAB® Code Configurator (https://www.microchip.com/mcc)

## Related Documentation
* AVR128DA48 [Product Family Page](https://www.microchip.com/wwwproducts/en/AVR128DA48)


# Firmware Operation
The data streamer firmware will output sensor data over the UART port with the following UART settings:

* Baudrate 115200
* Data bits 8
* Stop bits 1
* Parity None

# Firmware Configuration

## Sensor Selection
No support for other sensors currently.

## Streaming Format Selection
To select the data streaming format, set the `DATA_STREAMER_FORMAT` macro in `app_config.h` to the appropriate value as explained in the table below.

| Streaming Format | app_config.h Configuration Value |
| --- | --- |
| ASCII text | `#define DATA_STREAMER_FORMAT DATA_STREAMER_FORMAT_ASCII` |
| [MPLAB Data Visualizer](https://www.microchip.com/en-us/development-tools-tools-and-software/embedded-software-center/mplab-data-visualizer) stream | `#define DATA_STREAMER_FORMAT DATA_STREAMER_FORMAT_MDV` |
| [SensiML Simple Stream](https://sensiml.com/documentation/simple-streaming-specification/introduction.html) | `#define DATA_STREAMER_FORMAT DATA_STREAMER_FORMAT_SMLSS` |

## Sensor Configuration Parameters
High level sensor parameters like sample rate and axes selection can be configured by modifying the macro values defined in `firmware/src/app_config.h`. See the inline comments for further description.
