/*
 * testcan.c
 *
 *  Created on: Jun 29, 2026
 *      Author: ADMIN
 */
#include <stdint.h>
#include "stm32f407.h"
#include"gpio.h"
#include"spi.h"
#include<string.h>
#include"uart.h"
#include"i2c.h"
#include"timer.h"
#include"adc.h"
#include "bxcan.h"

CAN_Handle_t can;
void Can_init()
{
	can.canx=CAN1;
	can.cany.can_mode=1;
	can.cany.baudrate=250000;
	can.cany.auto_retransmission=0;
	CAN_Init(&can);
	CAN1->CAN_Control.IER|=1<<1|1<<2|1<<3|1<<4|1<<5|1<<6;

}

static inline void FPU_Enable(void)
{
    FPU_CPACR |= (0xF << 20);
    __asm volatile ("dsb 0xF");
    __asm volatile ("isb 0xF");
    FPU->FPCCR |= (1 << 31);   // ASPEN
    FPU->FPCCR |= (1 << 30);   // LSPEN
}
void CLK_init()
{
	    *((__vo uint32_t*)(0x40023C00))&=~(0x7);
		*((__vo uint32_t*)(0x40023C00))|=3;//60-90 mhz for hclk
		//enable hse
		RCC->CR|=1<<16;
		while(!(RCC->CR&(1<<17)));

		//choose PLL entry
		RCC->PLLCFGR|=1<<22;
		//PLL div M
		RCC->PLLCFGR&=~(0x3f);
		RCC->PLLCFGR|=8;//1mhz
		//PLL mul N
		RCC->PLLCFGR&=~(0x1ff<<6);
		RCC->PLLCFGR|=288<<6;//288mhz

		//PLL div q for otg fs 48mhz
		RCC->PLLCFGR&=~(0xf<<24);
		RCC->PLLCFGR|=6<<24;//48mhz
		//PLL div P for sys clock

		RCC->PLLCFGR&=~(0x3<<16);
		RCC->PLLCFGR|=0<<16;//144mhz
		// enable pll
		RCC->CR|=1<<24;
		while(!(RCC->CR&(1<<25)));
		//system clock select
		RCC->CFGR&=~(0x03);
		RCC->CFGR|=2;//pll selected as system clock
		while(((RCC->CFGR>>2)&0x03)!=0x02);//wait system clock sta en pll
		//AHB PRE
		RCC->CFGR&=~(0xf<<4);
		RCC->CFGR|=8<<4;//72mhz
		//APB2
		RCC->CFGR&=~(0x7<<13);
		RCC->CFGR|=4<<13;//36mhz
		//APB1
		RCC->CFGR&=~(0x7<<10);
		RCC->CFGR|=4<<10;//36mhz
}
void GPIO_config()
{
	GPIO_Handle_t gpiox;
	gpiox.gpiox=GPIOA;
	gpiox.gpioy.mode=GPIO_ALTFUNC;
	gpiox.gpioy.optype=GPIO_OUTPUT_PP;
	gpiox.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpiox.gpioy.pin=GPIO_PIN_11;
	gpiox.gpioy.alt_num=GPIO_PIN_AF9;
	gpiox.gpioy.pupd=GPIO_PU; // <--- ĐỔI THÀNH PULLUP ĐỂ ĐỒNG BỘ BUS
	GPIO_Init(&gpiox);
	gpiox.gpioy.pin=GPIO_PIN_12;
	gpiox.gpioy.pupd=GPIO_NO_PUPD; // TX giữ nguyên không cần pull
	GPIO_Init(&gpiox);
}
int main(void)
{
	CLK_init();
	GPIO_config();
	Can_init();
	CAN_Filter_config canfilter;
	canfilter.Filter_mode=0;
	canfilter.Filter_scale=1;
	canfilter.active=1;
	canfilter.assigment_fifo=0;
	canfilter.channel=0;
	canfilter.filter_id1=0x00<<21;
	canfilter.filter_id2=(0x07FF << 21);
	CAN_Filter_Init(&can,canfilter);
	canfilter.channel=1;
	canfilter.assigment_fifo=1;
	canfilter.filter_id1=(0x0123 << 21);
	canfilter.filter_id2=(0x07FF << 21);
	CAN_Filter_Init(&can,canfilter);
	NVIC_ConFig(20,ENABLE);
	NVIC_PRIORITY(20,1);
	NVIC_ConFig(21,ENABLE);
	NVIC_PRIORITY(21,1);
	//uint8_t buffer [8]={0x01,0x02};
	while(1)
	{
		CAN_TX_Transmit(&can,0x0000,(uint8_t*)("hello"),5);
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x123,(uint8_t*)("hahahaha"),strlen("hahahaha"));
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x0000,(uint8_t*)("chiu"),strlen("chiu"));
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x123,(uint8_t*)("ngao"),strlen("ngao"));
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x0000,(uint8_t*)("dumamay"),strlen("dumamay"));
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x123,(uint8_t*)("vl"),strlen("vl"));
		//for(int i=0;i<100000;i++);
		CAN_TX_Transmit(&can,0x0000,(uint8_t*)("cac"),strlen("cac"));
		//for(int i=0;i<100000;i++);
		/*CAN_RX_Transmit(&can,0);
		CAN_RX_Transmit(&can,1);
		for(int i=0;i<100000;i++);*/
	}
}
void CAN1_RX0_IRQHandler ()
{
	CAN_IRQ_RXFF0_Handle(&can);
}
void CAN1_RX1_IRQHandler ()
{
	CAN_IRQ_RXFF1_Handle(&can);
}
