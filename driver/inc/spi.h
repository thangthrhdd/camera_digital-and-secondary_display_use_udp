/*
 * spi.h
 *
 *  Created on: Nov 4, 2025
 *      Author: ADMIN
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_
#include <stm32f407.h>
typedef struct
{
	uint8_t mode_device;
	uint8_t mode;
	uint8_t data_frame;
	uint8_t frame_fomat;
	uint8_t baurate;
	uint8_t SSM;
	uint8_t CPOL;
	uint8_t CPHA;
	uint8_t SSI;
}SPI_ConFig_t;
typedef struct
{
	SPI_RegDef_t*spix;
	SPI_ConFig_t spiy;

	uint8_t*rx;
	uint8_t*tx;
	uint16_t tx_length;
	uint16_t rx_length;

	uint8_t status;

}SPI_Handle_t;
//mode device
#define SPI_MASTER_MODE 1
#define SPI_SLAVE_MODE 0
//mode
#define SPI_FULL_DUPLEX 0
#define SPI_HALF_DUPLEX 1
#define SPI_SIMPLEX 2
//data frame
#define SPI_8BIT 0
#define SPI_16BIT 1
//frame_format
#define SPI_MSB_FIRST 0
#define SPI_LSB_FIRST 1
//baudrate
#define SPI_BAUD_2 0
#define SPI_BAUD_4 1
#define SPI_BAUD_8 2
#define SPI_BAUD_16 3
#define SPI_BAUD_32 4
#define SPI_BAUD_64 5
#define SPI_BAUD_128 6
#define SPI_BAUD_256 7
//SSM
#define SPI_SSM_EN 1
#define SPI_SSM_DI 0
//CPOL
#define SPI_CPOL_EN 1
#define SPI_CPOL_DI 0
//CPHA
#define SPI_CPHA_EN 1
#define SPI_CPHA_DI 0
//IRQ
#define SPI_BSY_IN_TX 1
#define SPI_BSY_IN_RX 2
#define SPI_IDLE 0
#define SPI_COMPLETE 3

void SPI_CLK(SPI_RegDef_t*spix,uint8_t EnorDi);
void SPI_Init(SPI_Handle_t *spi_handle);
void SPI_Control(SPI_RegDef_t*spix,uint8_t EnorDi);
void SPI_SSI_ConFig(SPI_RegDef_t*spix,uint8_t EnorDi);
void SPI_SSOE_ConFig(SPI_RegDef_t*spix,uint8_t EnorDi);
void SPI_Master_Transmit(SPI_RegDef_t*spix,uint8_t*data,uint32_t legnth);
void SPI_Master_Receive(SPI_RegDef_t*spix,uint8_t*data,uint16_t length);
void SPI_Slave_Transmit(SPI_RegDef_t*spix,uint8_t*data,uint16_t length);
void SPI_Slave_Receive(SPI_RegDef_t*spix,uint8_t*data,uint16_t length);

uint8_t SPI_Master_Transmit_IT(SPI_Handle_t*spi_handle,uint8_t*data,uint16_t legnth);
uint8_t SPI_Master_Receive_IT(SPI_Handle_t*spi_handle,uint8_t*data,uint16_t legnth);
void SPI_IRQ_Handle(SPI_Handle_t*spi_handle);
#endif /* INC_SPI_H_ */
