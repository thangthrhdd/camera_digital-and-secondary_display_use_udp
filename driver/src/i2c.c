/*
 * i2c.c
 *
 *  Created on: Nov 12, 2025
 *      Author: ADMIN
 */
#include"i2c.h"
void I2C_CLK(I2C_RegDef_t *i2cx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(i2cx==I2C1)
		{
			I2C1_PCLK_EN();
		}
		else if(i2cx==I2C2)
		{
			I2C2_PCLK_EN();
		}
		else if(i2cx==I2C3)
		{
			I2C3_PCLK_EN();
		}
	}
	else
	{
		if(i2cx==I2C1)
		{
			I2C1_PCLK_DI();
		}
		else if(i2cx==I2C2)
		{
			I2C2_PCLK_DI();
		}
		else if(i2cx==I2C3)
		{
			I2C3_PCLK_DI();
		}
	}
}
void I2C_Init(I2C_Handle_t*i2c_handle)
{
	I2C_CLK(i2c_handle->i2cx,ENABLE);
	//ACK
	i2c_handle->i2cx->CR1&=~(0x01<<10);
	i2c_handle->i2cx->CR1|=(i2c_handle->i2cy.ACK_control<<10);
	//addr
	i2c_handle->i2cx->OAR1=i2c_handle->i2cy.addr_device<<1|(1<<14);
	//freq
	i2c_handle->i2cx->CR2=42;
	//ccr
	i2c_handle->i2cx->CCR&=~(0x03<<14);
	i2c_handle->i2cx->CCR|=i2c_handle->i2cy.duty_cycle<<14;
	uint32_t ccr=0;
	uint32_t fpclk=42000000;
	if(i2c_handle->i2cy.duty_cycle==I2C_SM_MODE)
	{
		ccr= ((uint32_t)fpclk/(i2c_handle->i2cy.speed*2));
	}
	else if(i2c_handle->i2cy.duty_cycle==I2C_FM_MODE_2)
	{
		ccr= ((uint32_t)fpclk/(i2c_handle->i2cy.speed*3));
	}
	else if(i2c_handle->i2cy.duty_cycle==I2C_FM_MODE_16_9)
	{
		ccr= ((uint32_t)fpclk/(i2c_handle->i2cy.speed*25));
	}
	i2c_handle->i2cx->CCR&=~(0xfff);
	i2c_handle->i2cx->CCR|=ccr&0xfff;
	//trise
	uint8_t trise=0;
	if(i2c_handle->i2cy.duty_cycle==I2C_SM_MODE)
	{
		trise=(uint8_t)((uint32_t)fpclk/(1000000U))+1;  //(*1000)/1000000000 //1000ns

	}
	else
	{
		trise=(uint8_t)((uint32_t)(fpclk*3)/(10000000U))+1;//300ns //I2C-bus specification and user manual//
	}
	i2c_handle->i2cx->TRISE=trise;
}
void I2C_Peripheral_Control(I2C_RegDef_t *i2cx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		i2cx->CR1|=1<<0;
	}
	else
	{
		i2cx->CR1&=~(1<<0);
	}
}
static void I2C_Start_Condition(I2C_RegDef_t *i2cx)
{
		i2cx->CR1|=1<<8;
		i2cx->CR1&=~(1<<9);
}
static void I2C_Stop_Condition(I2C_RegDef_t *i2cx)
{
		i2cx->CR1|=1<<9;
		i2cx->CR1&=~(1<<8);
}
static void I2C_ClearAddr(I2C_RegDef_t *i2cx,uint16_t length)
{
	uint32_t dummy_read;
	{
		dummy_read=i2cx->SR1;
		dummy_read=i2cx->SR2;
		(void)dummy_read;
	}
}
void I2C_Master_Transmit(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length,uint8_t addr)
{
	i2cx->CR1|=(0x01<<10);// enable ack
	I2C_Start_Condition(i2cx);
	while(!(i2cx->SR1&0x1));//wait SB flag set;
	i2cx->DR=((addr<<1)|0)&0xff;//send addr
  	while(!(i2cx->SR1&(1<<1)));//wait addr flag set;
	//clear sr1& sr2
	I2C_ClearAddr(i2cx,length);
	////////////////////

		while(length>0)
		{
			while(!(i2cx->SR1&(1<<7)));
			i2cx->DR=*data;
			data++;
			length--;
		}
	while(!(i2cx->SR1&(1<<7)));
  	while(!(i2cx->SR1&(1<<2)));
	I2C_Stop_Condition(i2cx);
}
void I2C_Master_Receive(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length,uint8_t addr)
{
	i2cx->CR1|=(0x01<<10);// enable ack
	uint16_t len=length;
	I2C_Start_Condition(i2cx);
	while(!(i2cx->SR1&0x1));//wait SB flag set;
	i2cx->DR=(addr<<1)|1;//send addr
	while(!(i2cx->SR1&(1<<1)));//wait addr flag set;
	//clear sr1& sr2
	if(length==1) i2cx->CR1&=~(0x01<<10);// if length=1 disable ack
	I2C_ClearAddr(i2cx,length);
	while(length>0)
	{
		while(!(i2cx->SR1&(1<<6)));//wait rxne set
		if(length==2)
		{
			i2cx->CR1&=~(0x01<<10);// disable ack
			I2C_Stop_Condition(i2cx);
		}
		if(len==1) I2C_Stop_Condition(i2cx);
		*data=i2cx->DR&0xff;
		data++;
		length--;
	}
	i2cx->CR1|=(0x01<<10);// enable ack

}
void I2C_Slave_Transmit(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length)
{
	I2C_Peripheral_Control(i2cx,ENABLE);
	i2cx->CR1|=(0x01<<10);// enable ack
	while(!(i2cx->SR1&(1<<1)));//wait addr flag set;
	I2C_ClearAddr(i2cx,length);
	while(length>0)
	{
		while(!(i2cx->SR1&(1<<7)));
		i2cx->DR=*data;
		data++;
		length--;
	}
	i2cx->CR1&=~(0x01<<10);// enable ack
	while(!(i2cx->SR1&(1<<10)));
	i2cx->SR1&=~(1<<10);

}
void I2C_Slave_Receive(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length)
{
	I2C_Peripheral_Control(i2cx,ENABLE);
	//i2cx->CR1&=~(0x01<<11);// dis pos
	i2cx->CR1|=(0x01<<10);// enable ack
	while(!(i2cx->SR1&(1<<1)));//wait addr flag set;
	I2C_ClearAddr(i2cx,length);

	while(length>0)
	{
		while(!(i2cx->SR1&(1<<6)));//wait rxne set
		*data=i2cx->DR&0xff;
		length--;
		data++;
	}
//	while(!(i2cx->SR1&(1<<6)));//wait rxne set
	while(!(i2cx->SR1&(1<<4)));//wait rxne set
}




//////////////////////////////////////////////////////////////////

uint8_t I2C_Master_Transmit_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length,uint8_t addr)
{
	uint8_t sta=i2c_handle->state;
	if(sta!=I2C_BSY_IN_TX)
	{
		i2c_handle->state=I2C_BSY_IN_TX;
		i2c_handle->i2c_tx=data;
		i2c_handle->tx_length=length;
		i2c_handle->slave_addr=addr;

		i2c_handle->i2cx->CR1|=(0x01<<10);//en ack
		I2C_Start_Condition(i2c_handle->i2cx);

		i2c_handle->i2cx->CR2|=1<<9;//Event interrupt enable
		i2c_handle->i2cx->CR2|=1<<10;// Buffer interrupt enable



	}
	return sta;
}
uint8_t I2C_Master_Receive_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length,uint8_t addr)
{
	uint8_t sta=i2c_handle->state;
	if(sta!=I2C_BSY_IN_RX)
	{
		i2c_handle->state=I2C_BSY_IN_RX;
		i2c_handle->i2c_rx=data;
		i2c_handle->rx_length=length;
		i2c_handle->slave_addr=addr;

		i2c_handle->i2cx->CR2|=1<<9;//Event interrupt enable
		i2c_handle->i2cx->CR2|=1<<10;// Buffer interrupt enable

		i2c_handle->i2cx->CR1|=(0x01<<10);//en ack
		I2C_Start_Condition(i2c_handle->i2cx);
	}
	return sta;
}
uint8_t I2C_Slave_Transmit_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length);
uint8_t I2C_Slave_Receive_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length);

void I2C_IRQ_Handle(I2C_Handle_t*i2c_handle)
{
	 if(i2c_handle->state == I2C_IDLE)
	       return;
	if(i2c_handle->i2cx->SR2&1) //master mode
	{
	uint8_t sta1=(i2c_handle->i2cx->SR1)&0x01;//sb flag
	uint8_t sta2=(i2c_handle->i2cx->CR2>>9)&0x01;
	if(sta1&&sta2)
	{
		i2c_handle->i2cx->DR=(i2c_handle->slave_addr<<1)|0;//send addr
		while(!(i2c_handle->i2cx->SR1&(1<<1)));//wait addr flag set;
		if(i2c_handle->i2cx->SR2&1)//master_mode
		{
			if(i2c_handle->state==I2C_BSY_IN_RX)
			{
				if(i2c_handle->rx_length==1)
				{
					 i2c_handle->i2cx->CR1&=~(0x01<<10);//dis ack
				}
			}
		}
		I2C_ClearAddr(i2c_handle->i2cx,i2c_handle->rx_length);
	}
	}
	uint8_t sta1=(i2c_handle->i2cx->SR1>>7)&0x01;
	uint8_t sta2=(i2c_handle->i2cx->CR2>>9)&0x01;
	if(sta1&&sta2)
	{
		if(i2c_handle->tx_length>0)
		{
			//while(!(i2c_handle->i2cx->SR1&(1<<7)));
			i2c_handle->i2cx->DR=(*i2c_handle->i2c_tx)&0xff;
			i2c_handle->i2c_tx++;
			i2c_handle->tx_length--;
		}
		else
		{
			//while(!(i2c_handle->i2cx->SR1&(1<<7)));
			while(!(i2c_handle->i2cx->SR1&(1<<2)));
			I2C_Stop_Condition(i2c_handle->i2cx);
			i2c_handle->state=I2C_IDLE;
			i2c_handle->i2cx->CR2&=~(0x01<<9);
			i2c_handle->i2cx->CR2&=~(0x01<<10);
			i2c_handle->tx_length=0;
			i2c_handle->i2c_tx=NULL;
		}
	}
}
