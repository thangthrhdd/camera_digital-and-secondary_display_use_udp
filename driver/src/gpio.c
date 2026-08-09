/*
 * gpio.c
 *
 *  Created on: Nov 3, 2025
 *      Author: ADMIN
 */
#include"gpio.h"
void GPIO_CLK(GPIO_RegDef_t*gpiox,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
	if(gpiox==GPIOA)
	{
		GPIOA_PCLK_EN();
	}
	else if(gpiox==GPIOB)
	{
		GPIOB_PCLK_EN();
	}
	else if(gpiox==GPIOC)
	{
		GPIOC_PCLK_EN();
	}
	else if(gpiox==GPIOD)
	{
		GPIOD_PCLK_EN();
	}
	else if(gpiox==GPIOE)
	{
		GPIOE_PCLK_EN();
	}
	else if(gpiox==GPIOF)
	{
		GPIOF_PCLK_EN();
	}
	else if(gpiox==GPIOG)
	{
		GPIOG_PCLK_EN();
	}
	else if(gpiox==GPIOH)
	{
		GPIOH_PCLK_EN();
	}
	else if(gpiox==GPIOI)
	{
		GPIOI_PCLK_EN();
	}
	}
	else
	{
		if(gpiox==GPIOA)
		{
			GPIOA_PCLK_DI();
		}
		else if(gpiox==GPIOB)
		{
			GPIOB_PCLK_DI();
		}
		else if(gpiox==GPIOC)
		{
			GPIOC_PCLK_DI();
		}
		else if(gpiox==GPIOD)
		{
			GPIOD_PCLK_DI();
		}
		else if(gpiox==GPIOE)
		{
			GPIOE_PCLK_DI();
		}
		else if(gpiox==GPIOF)
		{
			GPIOF_PCLK_DI();
		}
		else if(gpiox==GPIOG)
		{
			GPIOG_PCLK_DI();
		}
		else if(gpiox==GPIOH)
		{
			GPIOH_PCLK_DI();
		}
		else if(gpiox==GPIOI)
		{
			GPIOI_PCLK_DI();
		}
	}

}
void GPIO_Init(GPIO_Handle_t*gpio_handle)
{
	GPIO_CLK(gpio_handle->gpiox,ENABLE);
	gpio_handle->gpiox->PUPDR&=~(0x03<<(gpio_handle->gpioy.pin*2));
	gpio_handle->gpiox->PUPDR|=(gpio_handle->gpioy.pupd<<(gpio_handle->gpioy.pin*2));
	if(gpio_handle->gpioy.mode<=3)
	{
		//moder
		gpio_handle->gpiox->MODER&=~(0x03<<(gpio_handle->gpioy.pin*2));
		gpio_handle->gpiox->MODER|=(gpio_handle->gpioy.mode<<(gpio_handle->gpioy.pin*2));
		//optype
		gpio_handle->gpiox->OTYPER&=~(0x01<<(gpio_handle->gpioy.pin));
		gpio_handle->gpiox->OTYPER|=(gpio_handle->gpioy.optype<<(gpio_handle->gpioy.pin));
		//ospeed
		gpio_handle->gpiox->OSPEEDR&=~(0x03<<(gpio_handle->gpioy.pin*2));
		gpio_handle->gpiox->OSPEEDR|=(gpio_handle->gpioy.ospeed<<(gpio_handle->gpioy.pin*2));
		//pupd

		//ALTFUNC
		if(gpio_handle->gpioy.pin<8)
		{
			gpio_handle->gpiox->AFRL&=~(0x0f<<(gpio_handle->gpioy.pin*4));
			gpio_handle->gpiox->AFRL|=(gpio_handle->gpioy.alt_num<<(gpio_handle->gpioy.pin*4));
		}
		else
		{
			gpio_handle->gpiox->AFRH&=~(0x0f<<((gpio_handle->gpioy.pin%8)*4));
			gpio_handle->gpiox->AFRH|=(gpio_handle->gpioy.alt_num<<((gpio_handle->gpioy.pin%8)*4));
		}
	}
	else //exti
	{

		SYSCFG_PCLK_EN();
		if(gpio_handle->gpioy.mode==EXTI_RT)
		{
			EXTI->FTSR&=~(0x01<<gpio_handle->gpioy.pin);
			EXTI->RTSR|=(0x01<<gpio_handle->gpioy.pin);
		}
		else if(gpio_handle->gpioy.mode==EXTI_FT)
		{
			EXTI->FTSR|=(0x01<<gpio_handle->gpioy.pin);
			EXTI->RTSR&=~(0x01<<gpio_handle->gpioy.pin);
		}
		else if(gpio_handle->gpioy.mode==EXTI_RTFT)
		{
			EXTI->FTSR|=(0x01<<gpio_handle->gpioy.pin);
			EXTI->RTSR|=(0x01<<gpio_handle->gpioy.pin);
		}
		uint8_t row=gpio_handle->gpioy.pin/4;
		uint8_t col=gpio_handle->gpioy.pin%4;
		SYSCFG->EXTICR[row]&=~(0x0f<<(col*4));
		SYSCFG->EXTICR[row]|=GPIO_To_Code(gpio_handle->gpiox)<<(col*4);
		EXTI->IMR|=1<<gpio_handle->gpioy.pin;
		EXTI->PR|=1<<gpio_handle->gpioy.pin;
	}
}
void GPIO_WritePin(GPIO_RegDef_t*gpiox,uint8_t pin,uint8_t sta)
{
	if(sta==SET)
	{
		gpiox->BSRR&=~((1<<pin)<<16);
		gpiox->BSRR|=1<<pin;
	}
	else if(sta==RESET)
	{
		gpiox->BSRR&=~(1<<pin);
		gpiox->BSRR|=(1<<pin)<<16;
	}
}
uint8_t GPIO_ReadPin(GPIO_RegDef_t*gpiox,uint8_t pin)
{
   return (gpiox->IDR>>pin)&0x01;
}
void GPIO_TogglePin(GPIO_RegDef_t*gpiox,uint8_t pin)
{
	if((gpiox->ODR>>pin)&0x1)
	{
		gpiox->BSRR&=~(1<<pin);
		gpiox->BSRR|=(1<<pin)<<16;
	}
	else
	{
		gpiox->BSRR&=~((1<<pin)<<16);
		gpiox->BSRR|=1<<pin;
	}
}
void NVIC_ConFig(uint8_t IRQ_number,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
	if(IRQ_number<=31)
	{
		*((__vo uint32_t*)NVIC_ISER_BASEADDR)|=(1<<IRQ_number);
	}
	else if(IRQ_number>=32 && IRQ_number<=63)
	{
		*((__vo uint32_t*)(NVIC_ISER_BASEADDR+0x04))|=(1<<IRQ_number%32);
	}
	else if(IRQ_number>=64 && IRQ_number<=80)
	{
		*((__vo uint32_t*)(NVIC_ISER_BASEADDR+0x08))|=(1<<IRQ_number%64);
	}
	}
	else
	{
		if(IRQ_number<=31)
		{
			*((__vo uint32_t*)NVIC_ICER_BASEADDR)|=(1<<IRQ_number);
		}
		else if(IRQ_number>=32 && IRQ_number<=63)
		{
			*((__vo uint32_t*)(NVIC_ICER_BASEADDR+0x04))|=(1<<IRQ_number%32);
		}
		else if(IRQ_number>=64 && IRQ_number<=80)
		{
			*((__vo uint32_t*)(NVIC_ICER_BASEADDR+0x08))|=(1<<IRQ_number%64);
		}
	}
}
void NVIC_PRIORITY(uint8_t IRQ_number,uint8_t priority)
{
    uint8_t row=IRQ_number/4;
    uint8_t col=IRQ_number%4;
    *((__vo uint32_t*)(NVIC_IPR_BASEADDR+row*4))|=priority<<(8*col+4);
}
void GPIO_IRQ_Handle(uint8_t pin)
{
	if((EXTI->PR>>pin)&0x01)
	{
		EXTI->PR|=1<<pin;
	}
}
