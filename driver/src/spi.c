/*
 * spi.c
 *
 *  Created on: Nov 4, 2025
 *      Author: ADMIN
 */
#include"spi.h"
void SPI_CLK(SPI_RegDef_t*spix,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(spix==SPI1) SPI1_PCLK_EN();
		else if(spix==SPI2) SPI2_PCLK_EN();
		else if(spix==SPI3) SPI3_PCLK_EN();
	}
	else
	{
		if(spix==SPI1) SPI1_PCLK_DI();
		else if(spix==SPI2) SPI2_PCLK_DI();
		else if(spix==SPI3) SPI3_PCLK_DI();
	}
}
void SPI_Init(SPI_Handle_t *spi_handle)
{
	SPI_CLK(spi_handle->spix,ENABLE);
	//mode device
	spi_handle->spix->CR1&=~(0x01<<2);
	spi_handle->spix->CR1|=(spi_handle->spiy.mode_device<<2);
	//mode
	if(spi_handle->spiy.mode==SPI_FULL_DUPLEX)
	{
		spi_handle->spix->CR1&=~(1<<15);
		spi_handle->spix->CR1&=~(1<<10);

	}
	else if(spi_handle->spiy.mode==SPI_HALF_DUPLEX)
	{
		spi_handle->spix->CR1|=1<<15;
	}
	else if(spi_handle->spiy.mode==SPI_SIMPLEX)
	{
		spi_handle->spix->CR1&=~(1<<15);
		spi_handle->spix->CR1|=(1<<10);
	}
	//data frame
	spi_handle->spix->CR1&=~(0x01<<11);
	spi_handle->spix->CR1|=(spi_handle->spiy.data_frame<<11);
	//frame format
	spi_handle->spix->CR1&=~(0x01<<7);
	spi_handle->spix->CR1|=(spi_handle->spiy.frame_fomat<<7);
	//baudrate
	spi_handle->spix->CR1&=~(0x07<<3);
	spi_handle->spix->CR1|=(spi_handle->spiy.baurate<<3);
	//SSM
	spi_handle->spix->CR1&=~(0x01<<9);
	spi_handle->spix->CR1|=(spi_handle->spiy.SSM<<9);
	//CPOL
	spi_handle->spix->CR1&=~(0x01<<1);
	spi_handle->spix->CR1|=(spi_handle->spiy.CPOL<<1);
	//CPHA
	spi_handle->spix->CR1&=~(0x01<<0);
	spi_handle->spix->CR1|=(spi_handle->spiy.CPHA<<0);
	//spi_handle->spix->CR1|=1<<6;
	spi_handle->spix->CR1&=~(0x01<<8);
	spi_handle->spix->CR1|=(spi_handle->spiy.SSI<<8);
}
void SPI_Control(SPI_RegDef_t*spix,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		spix->CR1|=1<<6;
	}
	else spix->CR1&=~(1<<6);
}
void SPI_SSI_ConFig(SPI_RegDef_t*spix,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		spix->CR1|=1<<8;
	}
	else spix->CR1&=~(1<<8);
}
void SPI_SSOE_ConFig(SPI_RegDef_t*spix,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		spix->CR2|=1<<2;
	}
	else spix->CR2&=~(1<<2);
}
/*void SPI_Master_Transmit(SPI_RegDef_t*spix,uint8_t*data,uint32_t length)
{
	while(length>0)
	{
		while(!(spix->SR&(1<<1)));//wait until tx buffer empty
		if(spix->CR1&(1<<11))//16bit dff
		{
			spix->DR=((uint16_t)*data);
			(uint16_t*)data++;
			length--;
		}
		else //8bit
		{
			spix->DR=*data&0xff;
			data++;
			length--;
		}
	}
	while(spix->SR&(1<<7));//wait bsy until 0
}*/
void SPI_Master_Transmit(SPI_RegDef_t*spix,uint8_t*data,uint32_t length)
{
	while(length>0)
	{
		while(!(spix->SR&(1<<1)));//wait until tx buffer empty
		if(spix->CR1&(1<<11))//16bit dff
		{
			spix->DR=((uint16_t)*data);
			(uint16_t*)data++;
			length--;
		}
		else //8bit
		{
			spix->DR=*data&0xff;
			data++;
			length--;
		}
	}
	while(spix->SR&(1<<7));//wait bsy until 0

    // --- THÊM PHẦN NÀY ĐỂ TRỊ TẬN GỐC LỖI CHUNG SPI ---
    // Hút sạch dữ liệu rác sinh ra trong quá trình truyền
    volatile uint32_t temp;
    while(spix->SR & (1<<0)) {
        temp = spix->DR;
    }
    // Xóa cờ Overrun (OVR) theo đúng quy trình của STM32: Đọc DR rồi đọc SR
    temp = spix->DR;
    temp = spix->SR;
    (void)temp; // Tránh warning unused variable
    // --------------------------------------------------
}
void SPI_Master_Receive(SPI_RegDef_t*spix,uint8_t*data,uint16_t length)
{
	while(length>0)
	{
		while(!(spix->SR&(1<<0)));//wait until rx buffer empty
		if(spix->CR1&(1<<11))//16bit dff
		{
			*(uint16_t*)data=spix->DR;
			(uint16_t*)data++;
			length-=2;
		}
		else //8bit
		{
			*data=spix->DR&0xff;
			data++;
			length--;
		}
	}
}
void SPI_Slave_Transmit(SPI_RegDef_t*spix,uint8_t*data,uint16_t length);
void SPI_Slave_Receive(SPI_RegDef_t*spix,uint8_t*data,uint16_t length);

uint8_t SPI_Master_Transmit_IT(SPI_Handle_t*spi_handle,uint8_t*data,uint16_t length)
{
	uint8_t sta=spi_handle->status;
	if(sta!=SPI_BSY_IN_TX)
	{
		spi_handle->status=SPI_BSY_IN_TX;
		spi_handle->tx=data;
		spi_handle->tx_length=length;
		spi_handle->spix->CR2|=1<<7;
	}
	return sta;
}
uint8_t SPI_Master_Receive_IT(SPI_Handle_t*spi_handle,uint8_t*data,uint16_t length)
{
	uint8_t sta=spi_handle->status;
	if(sta!=SPI_BSY_IN_RX)
	{
		spi_handle->status=SPI_BSY_IN_RX;
		spi_handle->rx=data;
		spi_handle->rx_length=length;
		spi_handle->spix->CR2|=1<<6;
	}
	return sta;
}
void SPI_IRQ_Handle(SPI_Handle_t*spi_handle)
{
	uint8_t sta1=(spi_handle->spix->CR2>>7)&0x1;//txeie
	uint8_t sta2=(spi_handle->spix->SR>>1)&0x1;//txe
	if(sta1 && sta2)
	{
		if(spi_handle->tx_length>0)
		{
			while(!(spi_handle->spix->SR&(1<<1)));//wait until tx buffer empty
			if(spi_handle->spix->CR1&(1<<11))//16bit dff
			{
				spi_handle->spix->DR=((uint16_t)*spi_handle->tx);
				(uint16_t*)spi_handle->tx++;
				spi_handle->tx_length--;
			}
			else //8bit
			{
				spi_handle->spix->DR=(*spi_handle->tx)&0xff;
				spi_handle->tx++;
				spi_handle->tx_length--;
			}
			while(spi_handle->spix->SR&(1<<7));//wait bsy until 0
		}
		else
		{

			spi_handle->tx_length=0;
			spi_handle->tx=NULL;
			spi_handle->spix->CR2&=~(1<<7);
			spi_handle->status=SPI_IDLE;
		}
	}
}
