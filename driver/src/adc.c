/*
 * adc.c
 *
 *  Created on: Dec 2, 2025
 *      Author: ADMIN
 */
#include "adc.h"
void ADC_Clock(ADC_RegDef_t*adcx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(adcx==ADC1) ADC1_PCLK_EN();
		else if(adcx==ADC2) ADC2_PCLK_EN();
		else if(adcx==ADC3) ADC3_PCLK_EN();
	}
	else
	{
		if(adcx==ADC1) ADC1_PCLK_DI();
		else if(adcx==ADC2) ADC2_PCLK_DI();
		else if(adcx==ADC3) ADC3_PCLK_DI();
	}
}
void ADC_Init(ADC_Handle_t*adc_handle)
{
	ADC_Clock(adc_handle->adcx,ENABLE);
	uint32_t reg=0;
	//réolution
	adc_handle->adcx->CR1&=~(0x03<<24);
	adc_handle->adcx->CR1|=(adc_handle->adcy.resolution<<24);



	//sampling
	uint8_t row,col;
	row=adc_handle->adcy.channel/10;
	col=adc_handle->adcy.channel%10;
	adc_handle->adcx->SMPR[1-row]&=~(0x07<<col*3);
	adc_handle->adcx->SMPR[1-row]|=(adc_handle->adcy.cycle<<col*3);

	row=(adc_handle->adcy.rank-1)/6;
	col=(adc_handle->adcy.rank-1)%6;
	adc_handle->adcx->SQR[2-row]&=~(0x1f<<col*5);
	adc_handle->adcx->SQR[2-row]|=(adc_handle->adcy.channel<<col*5);
	adc_handle->adcx->SQR[0]&=~(0x1f<<20);
	adc_handle->adcx->SQR[0]|=(adc_handle->adcy.length_of_cvst<<20);
	ADC1->CR2|=1<<10;
	ADC1->CR1|=1<<8;

	//sigle mode
	adc_handle->adcx->CR2|=(1<<0);

}
void ADC_Read(ADC_RegDef_t*adcx,uint8_t length,uint16_t *buff)
{
	adcx->SR&=~(1<<1);
	adcx->CR2|=1<<30;
	for(int i=0;i<length;i++)
	{
	while(!(adcx->SR&(1<<1)));
	buff[i]=adcx->DR&(0xfff);
	}
	adcx->CR2&=~(1<<30);
}
void ADC_IRQ_Handle(ADC_Handle_t*adc_handle)
{

}
