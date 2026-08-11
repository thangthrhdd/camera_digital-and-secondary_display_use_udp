/*
 * sd.h
 *
 *  Created on: Jul 16, 2026
 *      Author: ADMIN
 */

#ifndef INC_SD_H_
#define INC_SD_H_
#include "stm32f407.h"
#include"spi.h"
#include "gpio.h"
#include "tftcolor.h"
extern volatile uint8_t sd_mounted;
typedef struct __attribute__((packed))
{
	uint8_t cmd;
	uint32_t arg;
	uint8_t crc;
}SD_DATA_CMD;
typedef struct
{
	union
	{
	SD_DATA_CMD cmd;
	uint8_t buffer[6];
	}
}SD_PACKET;
extern SD_PACKET sd;
// --- API EVENT CỦA JOYSTICK ---
typedef enum {
    EVENT_NONE = 0,
    EVENT_LEFT,
    EVENT_RIGHT,
    EVENT_UP,
    EVENT_DOWN,
    EVENT_SELECT // Sẽ dùng Gạt Lên/Gạt Phải làm nút Chọn
} MenuEvent_t;
#define SPI_PORT SPI2//SPI1
#define CS_SET()  GPIO_WritePin(GPIOB,GPIO_PIN_12,SET);//GPIO_WritePin(GPIOD,GPIO_PIN_2,SET);
#define CS_RESET() GPIO_WritePin(GPIOB,GPIO_PIN_12,RESET); //GPIO_WritePin(GPIOD,GPIO_PIN_2,RESET);
uint8_t SD_Init();
uint8_t SD_CMD(uint8_t cmd,uint32_t arg);
uint8_t SD_Write_Sector(uint32_t sector_address, uint8_t *data_buffer);
uint8_t SD_Read_Sector(uint32_t sector_address, uint8_t *data_buffer);
uint8_t SPI_Transfer(uint8_t data,SPI_RegDef_t * spiport);
void Render_Menu(SPI_Handle_t* hspi);
uint8_t adc_handle(uint16_t bufff[5]);
#endif /* INC_SD_H_ */
