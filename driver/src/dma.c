/*
 * dma.c
 *
 *  Created on: Jul 5, 2026
 *      Author: ADMIN
 */
#include"dma.h"
void DMA_CLK(DMA_RegDef_t * dmax,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(dmax==DMA1)
		{
			DMA1_PCLK_EN();
		}
		else if(dmax==DMA2)
		{
			DMA2_PCLK_EN();
		}
	}
	else
	{
		if(dmax==DMA1)
		{
			DMA1_PCLK_DI();
		}
		else if(dmax==DMA2)
		{
			DMA2_PCLK_DI();
		}
	}
}
void DMA_Init(DMA_Handle_t* dma)
{
	DMA_CLK(dma->dmax,ENABLE);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR&=~(0xfffffff);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.channel&0x07)<<25);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Mem_burst_transfer&0x03)<<23);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Peri_burst_transfer&0x03)<<21);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.mode&0x03)<<18);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.priority&0x03)<<16);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.offset_size)<<15);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.mem_data_size&0x03)<<13);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.per_data_size&0x03)<<11);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Memory_mode)<<10);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Peripheral_mode)<<9);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.circular_mode)<<8);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Data_transfer_direction&0x03)<<6);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=((dma->dmay.Peripheral_flow_controller)<<5);


	dma->dmax->DMA_MEM[dma->dmay.stream_num].SNDTR&=~(0xffff);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SNDTR|=(dma->dmay.number_of_data)&0xffff;

	dma->dmax->DMA_MEM[dma->dmay.stream_num].SPAR=(dma->dmay.per_addr);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SM0AR=(dma->dmay.mem0_addr);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SM1AR=(dma->dmay.mem1_addr);

	dma->dmax->DMA_MEM[dma->dmay.stream_num].SFCR&=~(0x07);
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SFCR|=(dma->dmay.FIFO_threshold)&0x03;
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SFCR|=dma->dmay.direct_mode_dis<<2;
	dma->dmax->DMA_MEM[dma->dmay.stream_num].SCR|=1<<0;

}

