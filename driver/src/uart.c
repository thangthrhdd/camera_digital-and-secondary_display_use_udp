/*
 * uart.c
 *
 *  Created on: Nov 7, 2025
 *      Author: ADMIN
 */
#include "uart.h"
void UART_CLK(UART_RegDef_t* uartx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(uartx==USART1) USART1_PCLK_EN();
		else if(uartx==USART6) USART6_PCLK_EN();
		else if(uartx==UART2) UART2_PCLK_EN();
		else if(uartx==UART3) UART3_PCLK_EN();
		else if(uartx==UART4) UART4_PCLK_EN();
		else if(uartx==UART5) UART5_PCLK_EN();
	}
	else
	{
		if(uartx==USART1) USART1_PCLK_DI();
		else if(uartx==USART6) USART6_PCLK_DI();
		else if(uartx==UART2) UART2_PCLK_DI();
		else if(uartx==UART3) UART3_PCLK_DI();
		else if(uartx==UART4) UART4_PCLK_DI();
		else if(uartx==UART5) UART5_PCLK_DI();
	}
}
static uint32_t UART_Baudrate_caculation(UART_RegDef_t* uartx,uint32_t baudrate,uint8_t over_samp)
{
	uint32_t tmp=0;
	uint32_t fpclk=16000000;
	uint32_t x= (fpclk*100)/(8*(2-over_samp)*baudrate);
	uint32_t mantisa=x/100;
	uint32_t fraction=(((8*(1+(1-over_samp)))*(x%100))/100)+1;
	if(over_samp==1)
	tmp= (mantisa<<4)|(fraction&0x07);
	else tmp= (mantisa<<4)|(fraction&0x0f);
	return tmp;
}
void UART_Init(UART_Handle_t*uart_handle)
{
	UART_CLK(uart_handle->uartx,ENABLE);
	uart_handle->uartx->CR1|=1<<13;
	//mode
	uart_handle->uartx->CR1&=~(0x03<<2);
	uart_handle->uartx->CR1|=(uart_handle->uarty.mode_device<<2);
	//M wword
	uart_handle->uartx->CR1&=~(0x01<<12);
	uart_handle->uartx->CR1|=(uart_handle->uarty.word_length<<12);
	//parity
	uart_handle->uartx->CR1&=~(0x03<<9);
	uart_handle->uartx->CR1|=(uart_handle->uarty.parity_cs<<9);
	//over samp
	uart_handle->uartx->CR1&=~(0x01<<15);
	uart_handle->uartx->CR1|=(uart_handle->uarty.over_sample<<15);
	//stop bit
	uart_handle->uartx->CR2&=~(0x03<<12);
	uart_handle->uartx->CR2|=(uart_handle->uarty.stop_bit<<12);
	//HW flow
	uart_handle->uartx->CR3&=~(0x03<<8);
	uart_handle->uartx->CR3|=(uart_handle->uarty.hardware_flow_control<<8);
	//baud rate
	uint32_t tmp=UART_Baudrate_caculation(uart_handle->uartx,uart_handle->uarty.baud_rate,uart_handle->uarty.over_sample);
	uart_handle->uartx->BRR=tmp;
}
void UART_Control(UART_RegDef_t* uartx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		uartx->CR1|=1<<13;
	}
	else uartx->CR1&=~(1<<13);
}
void UART_Transmit(UART_RegDef_t* uartx,uint8_t*data,uint16_t length)
{
	while(length>0)
	{
		while(!(uartx->SR&(1<<7)));
		if(uartx->CR1&(1<<12))//9bit
		{
			if(uartx->CR1&(1<<10))//parity en
			{
				uartx->DR= *data;
				data++;
			}
			else
			{
				uartx->DR=*((uint16_t*)data)&0x1ff;
				(uint16_t*)data++;
			}
		}
		else
		{
			if(uartx->CR1&(1<<10))//parity en
			{
				uartx->DR= (*data&0x7f);
			}
			else
			{
				//uartx->DR&=~(0xffff);
				uartx->DR=*data&0xff;
			}
			data++;
		}
		length--;
	}
	while(!(uartx->SR&(1<<6)));//wait tc en
}
void UART_Receive(UART_RegDef_t* uartx,uint8_t*data,uint16_t length)
{
	while(length>0)
	{
		while(!(uartx->SR&(1<<5)));
		if(uartx->CR1&(1<<12))//9bit
		{
			if(uartx->CR1&(1<<10))//parity en
			{
				*data=uartx->DR&(0xff);
				data++;
			}
			else
			{
				*((uint16_t*)data)=uartx->DR&0x1ff;
				(uint16_t*)data++;
			}
		}
		else
		{
			if(uartx->CR1&(1<<10))//parity en
			{
				*data=uartx->DR &0x7f;
			}
			else
			{
				//uartx->DR&=~(0xffff);
				*data=uartx->DR&0xff;
			}
			data++;
		}
		length--;
	}

}
uint8_t UART_Transmit_IT(UART_Handle_t*uart_handle,uint8_t*data,uint16_t length)
{
	uint8_t sta=uart_handle->state;
	if(sta!=UART_BSY_TX)
	{
		uart_handle->state=UART_BSY_TX;
		uart_handle->tx_data=data;
		uart_handle->tx_length=length;
		uart_handle->uartx->CR1|=1<<7|1<<6;
	}
	return sta;
}
uint8_t UART_Receive_IT(UART_Handle_t*uart_handle,uint8_t*data,uint16_t length)
{
	uint8_t sta=uart_handle->state;
	if(sta!=UART_BSY_RX)
	{
		uart_handle->state=UART_BSY_RX;
		uart_handle->tx_data=data;
		uart_handle->tx_length=length;
		uart_handle->uartx->CR1|=1<<5;
	}
	return sta;
}
void UART_IRQ_Handle(UART_Handle_t*uart_handle)
{

	//TC
  uint8_t sta1=(uart_handle->uartx->CR1>>6)&0x01;
  uint8_t sta2=(uart_handle->uartx->SR>>6)&0x01;
  if(sta1&&sta2)
  {
	  if(uart_handle->state==UART_BSY_TX)
	  {
		 if(uart_handle->tx_length==0)
		 {
			 uart_handle->tx_length=0;
	          uart_handle->tx_data=NULL;
	          uart_handle->uartx->CR1&=~(0x01<<6);
	          uart_handle->uartx->SR&=~(0x01<<6);
	          uart_handle->state=UART_IDLE;
		 }
	  }
  }
  //TX
   sta1=(uart_handle->uartx->CR1>>7)&0x01;
   sta2=(uart_handle->uartx->SR>>7)&0x01;
  if(sta1&&sta2)
  {
	  if(uart_handle->tx_length>0)
	  {
			while(!(uart_handle->uartx->SR&(1<<7)));
			if(uart_handle->uartx->CR1&(1<<12))//9bit
			{
				if(uart_handle->uartx->CR1&(1<<10))//parity en
				{
					uart_handle->uartx->DR=*uart_handle->tx_data&(0xff);
					uart_handle->tx_data++;
				}
				else
				{
					uart_handle->uartx->DR=*((uint16_t*)uart_handle->tx_data)&0x1ff;
					(uint16_t*)uart_handle->tx_data++;
				}
			}
			else
			{
				if(uart_handle->uartx->CR1&(1<<10))//parity en
				{
					uart_handle->uartx->DR=*uart_handle->tx_data &0x7f;
				}
				else
				{
					//uartx->DR&=~(0xffff);
					uart_handle->uartx->DR=*uart_handle->tx_data&0xff;
				}
				uart_handle->tx_data++;
			}
			uart_handle->tx_length--;
	  }
	  else
	  {
		  uart_handle->uartx->CR1&=~(0x01<<7);
	  }
  }
}
