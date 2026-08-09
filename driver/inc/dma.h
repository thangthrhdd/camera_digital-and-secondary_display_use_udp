/*
 * dma.h
 *
 *  Created on: Jul 5, 2026
 *      Author: ADMIN
 */

#ifndef INC_DMA_H_
#define INC_DMA_H_
#include "stm32f407.h"
typedef struct
{
	uint8_t stream_num;
	uint8_t channel;
	uint8_t Mem_burst_transfer;
	uint8_t Peri_burst_transfer;
	uint8_t mode;
	uint8_t priority;
	uint8_t offset_size;
	uint8_t mem_data_size;
	uint8_t per_data_size;
	uint8_t Memory_mode;
	uint8_t  Peripheral_mode;
	uint8_t circular_mode;
	uint8_t Data_transfer_direction;
	uint8_t Peripheral_flow_controller;

	uint16_t number_of_data;
	uint32_t per_addr;
	uint32_t mem0_addr;
	uint32_t mem1_addr;
	uint8_t direct_mode_dis;
	uint8_t FIFO_threshold;

}DMA_Config_t;
typedef struct
{
	DMA_RegDef_t * dmax;
	DMA_Config_t dmay;
}DMA_Handle_t;

void DMA_CLK(DMA_RegDef_t * dmax,uint8_t EnorDi);
void DMA_Init(DMA_Handle_t* dma);


#endif /* INC_DMA_H_ */
