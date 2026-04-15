#ifndef PINOUT_H
#define PINOUT_H
/*
Seed Studio WT32-SC01 Plus 
ESP32 development board With 3.5 Inch LCD IPS Display Touch Screen
*/

#define TXD0 42
#define RXD0 1

#define ESP_TXD TXD0
#define ESP_RXD RXD0

/*SD Card*/
//DI = MOSI; DO = MISO
#define SD_CS 41
#define SD_DI 40
#define SD_CLK 39
#define SD_DO 38    

/*LCD*/
#define LCD_BL 45
#define LCD_RESET 4
#define LCD_RS 0
#define LCD_WR 47
#define LCD_TE 48
#define LCD_DB0 9
#define LCD_DB1 46
#define LCD_DB2 3
#define LCD_DB3 8
#define LCD_DB4 18
#define LCD_DB5 17
#define LCD_DB6 16
#define LCD_DB7 15

/*TouchPad*/
#define TP_INT 7
#define TP_SDA 6
#define TP_SCL 5
#define TP_RST 4

/*USB*/
#define USB_DN 19
#define USB_DP 20

/*Audio*/
#define LRCK 35
#define BCLK 36
#define DOUT 37

/*RS-485*/
#define RXD_IO 1
#define RTS_IO 2
#define TXD_IO 42



#endif