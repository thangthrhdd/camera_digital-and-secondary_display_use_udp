/*
 * tftcolor.h
 *
 *  Created on: Jul 4, 2026
 *      Author: ADMIN
 */

#ifndef INC_TFTCOLOR_H_
#define INC_TFTCOLOR_H_
#include"spi.h"
// Định nghĩa màu sắc (RGB565)
#define TFT_COLOR_WHITE   0xFFFF
#define TFT_COLOR_BLACK   0x0000
#define TFT_COLOR_RED     0xF800
/*
 //* tfteth
#define TFT_DC_CMD()      GPIO_WritePin(GPIOB,6,RESET)
#define TFT_DC_DATA()     GPIO_WritePin(GPIOB,6,SET)
#define TFT_CS_LOW()      GPIO_WritePin(GPIOB,9,RESET)
#define TFT_CS_HIGH()     GPIO_WritePin(GPIOB,9,SET)
#define TFT_RST_LOW()     GPIO_WritePin(GPIOB,7,RESET)
#define TFT_RST_HIGH()    GPIO_WritePin(GPIOB,7 ,SET)*/

//tft cam
#define TFT_DC_CMD()      GPIO_WritePin(GPIOD,6,RESET)
#define TFT_DC_DATA()     GPIO_WritePin(GPIOD,6,SET)
#define TFT_CS_LOW()      GPIO_WritePin(GPIOD,5,RESET)
#define TFT_CS_HIGH()     GPIO_WritePin(GPIOD,5,SET)
#define TFT_RST_LOW()     GPIO_WritePin(GPIOD,7,RESET)
#define TFT_RST_HIGH()    GPIO_WritePin(GPIOD,7 ,SET)


void data_p(SPI_Handle_t* spi, uint8_t data);
void cmd(SPI_Handle_t*spi,uint8_t data);
void tft_init(SPI_Handle_t* spi);
void tft_set_window(SPI_Handle_t* spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void tft_write_char(SPI_Handle_t* spi, uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color);
void tft_write_string(SPI_Handle_t* spi, uint16_t x, uint16_t y, char* str, uint16_t color, uint16_t bg_color);
void tft_clear(SPI_Handle_t* spi, uint16_t color);
void tft_draw_image(SPI_Handle_t *hspi, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* image_data);
#endif /* INC_TFTCOLOR_H_ */
