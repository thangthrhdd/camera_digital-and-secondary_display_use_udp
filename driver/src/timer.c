/*
 * timer.c
 *
 *  Created on: Nov 30, 2025
 *      Author: ADMIN
 */
#include"timer.h"
void TIM_CLOCK(TIM_RegDef_t*timx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(timx==TIM1) TIM1_PCLK_EN();
		else if(timx==TIM2) TIM2_PCLK_EN();
		else if(timx==TIM8) TIM8_PCLK_EN();
	}
	else
	{
		if(timx==TIM1) TIM1_PCLK_DI();
		else if(timx==TIM2) TIM2_PCLK_DI();
		else if(timx==TIM8) TIM8_PCLK_DI();
	}
}
void TIM_Init(TIM_Handle_t*tim_handle)
{
	TIM_CLOCK(tim_handle->timx,ENABLE);
	//mode
	tim_handle->timx->CCMR[0]&=~(0x7<<4);
	tim_handle->timx->CCMR[0]|=(tim_handle->timy.mode<<4);
	//arr control
	tim_handle->timx->CR1&=~(0x1<<7);
	tim_handle->timx->CR1|=(tim_handle->timy.ARR_control<<7);
	//dir control
	tim_handle->timx->CR1&=~(0x1<<4);
	tim_handle->timx->CR1|=(tim_handle->timy.DIR_control<<4);
	//one pulse control
	tim_handle->timx->CR1&=~(0x1<<3);
	tim_handle->timx->CR1|=(tim_handle->timy.one_pulse<<3);

//	tim_handle->timx->CNT=tim_handle->timy.CNT;
//	tim_handle->timx->ARR=tim_handle->timy.ARR;
	tim_handle->timx->PSC=tim_handle->timy.psc;
}
void TIM_Delay_us (TIM_RegDef_t*timx,uint32_t us,uint16_t arr)
{
	timx->CR1|=0x03<<8;
	timx->ARR=arr;
	timx->PSC=15999;
	timx->EGR|=1;
	timx->SR &= ~(1<<0);
	timx->CR1 |= 1;           // start
	while(!(timx->SR & (1<<0)));  // wait update
	timx->SR &=~1;      // clear flag
	timx->CR1 &= ~1;          // stop timer
}
void TIM_PWM (TIM_RegDef_t*timx,uint32_t period,uint8_t duty_cycle,uint8_t channel);

void TIM_IRQ_Handle(TIM_Handle_t*tim_handle)
{
	if(tim_handle->timx->SR&(1<<0))
	{
		tim_handle->timx->ARR=1;
		tim_handle->timx->PSC=15999;
		tim_handle->timx->SR &=~1;      // clear flag
	}
}
