## ch32v307vct6 camera
# description
    This project uses a WCH CH32V307VCT6 chip to transmit real-time video to a PC.
    The camera module is an OV2640, and local video display is achieved via a 128x160 RGB LCD driven by an ST7735S chip.

## Note 
  Only the Ld and User files are included in this repository. Other files, such as the Core folder, need to be configured in the project by yourself.



##
/********************************** (C) COPYRIGHT *******************************
* Author             : KAZE1022 : OV2640 -> 1.8-inch 128x160 ST7735S (SPI)
* Version            : V2.0.0
* Date               : 2025/04/06
* Description        : OV2640 video stream display + parameter diagnostic page + pixel calibration screen.
*
*  1. Camera wiring (same as the original WCH example)
*    D0--PA9    D1--PA10   D2--PC8    D3--PC9    D4--PC11   D5--PB6    D6--PB8    D7--PB9
*    D8--PC10   D9--PC12   D10--PD6   D11--PD2
*    DPCLK--PA6  HERF--PA4  SYNC--PA5
*    SDCLK--PB10 (SCCB SCL)   SDA--PB11 (SCCB SDA)
*    RESET--PC13  DPWDN--PC3   (the NC pin on the module can be left floating)
*
*  2. LCD wiring (1.8-inch 128x160 ST7735S, 8-pin SPI module)
*    VCC--3.3V  GND--GND  SCL--PB13(SPI2_SCK)  SDA--PB15(SPI2_MOSI)
*    RST--PC4   DC--PB14  CS--PB12             BLK--PC5(active high to turn on)
*
*  3. Power-on sequence
*    1. Pixel calibration screen (color bars + grayscale + black/white blocks + border)   about 2 seconds
*    2. Parameter diagnostic page (frame/line/dropped line/FIFO/sharpness/data samples)  about 6 seconds
*    3. Camera real-time screen (full screen)
*
*  4. Serial port: UART1(PA9/PA10) is occupied by DVP D0/D1, debug printing uses UART2(PA2).
*     DEBUG=DEBUG_UART2 is defined in the project; when undefined, DBG is automatically disabled and will not hang.
*********************************************************************************/



