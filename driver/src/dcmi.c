/*
 * dcmi.c
 *
 *  Created on: Aug 1, 2026
 *      Author: ADMIN
 */
#include "dcmi.h"
void DCMI_Init(DCMI_Handle_t *dcmi_handle)
{
	DCMI_PCLK_EN();
	dcmi_handle->dcmix->CR&=~(0x7ff<<1);
	//captrue mode
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.captrue_mode<<1;
	//Crop feature
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.Crop_feature<<2;
	//JPEG format
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.JPEG_format<<3;
	//Embedded synchronization select
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.hard_or_embed_mode<<4;
	//clock polarity
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.Pixel_clock_polarity<<5;
	//Horizontal synchronization polarity
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.Horizontal_syn_polarity<<6;
	//Vertical synchronization polarity
	dcmi_handle->dcmix->CR|=dcmi_handle->dcmiy.Vertical_syn_polarity<<7;
	//Frame capture rate control
	dcmi_handle->dcmix->CR|=(dcmi_handle->dcmiy.Frame_capture_rate&0x03)<<8;
	//Extended data mode
	dcmi_handle->dcmix->CR|=(dcmi_handle->dcmiy.data_mode&0x03)<<10;
	//dcmi_handle->dcmix->CR|=1<<14;
}

void DCMI_CAPTRUE(DCMI_Handle_t *dcmi_handle,uint8_t enordi)
{
	if(enordi==ENABLE)
	{
		dcmi_handle->dcmix->CR|=1<<14;
		dcmi_handle->dcmix->CR|=1<<0;

	}
	else
		{
			dcmi_handle->dcmix->CR&=~(1<<0);
			dcmi_handle->dcmix->CR&=~(1<<14);

		}
}
