/*
 * i2c.h
 *
 *  Created on: Nov 12, 2025
 *      Author: ADMIN
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_
#include"stm32f407.h"

typedef struct
{
	uint16_t addr_device;
	uint32_t speed;
	uint8_t duty_cycle;
	uint8_t ACK_control;

}I2C_ConFig_t;

typedef struct
{
	I2C_RegDef_t *i2cx;
	I2C_ConFig_t i2cy;

	uint8_t *i2c_tx;
	uint8_t*i2c_rx;
	uint16_t tx_length;
	uint16_t rx_length;
	uint8_t slave_addr;
	uint8_t state;
}I2C_Handle_t;

//speed
#define I2C_SM_100Khz  100000U
#define I2C_FM_400Khz  400000U
//duty cycle
#define I2C_SM_MODE 0
#define I2C_FM_MODE_2 2
#define I2C_FM_MODE_16_9 3
//ACK
#define I2C_ACK_EN 0
#define I2C_ACK_DI 1

//define Irq
#define I2C_IDLE 0
#define I2C_BSY_IN_RX 1
#define I2C_BSY_IN_TX 2
#define I2C_CMPLTE 3


void I2C_CLK(I2C_RegDef_t *i2cx,uint8_t EnorDi);
void I2C_Init(I2C_Handle_t*i2c_handle);
void I2C_Peripheral_Control(I2C_RegDef_t *i2cx,uint8_t EnorDi);
void I2C_Master_Transmit(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length,uint8_t addr);
void I2C_Master_Receive(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length,uint8_t addr);
void I2C_Slave_Transmit(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length);
void I2C_Slave_Receive(I2C_RegDef_t *i2cx,uint8_t *data,uint16_t length);

uint8_t I2C_Master_Transmit_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length,uint8_t addr);
uint8_t I2C_Master_Receive_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length,uint8_t addr);
uint8_t I2C_Slave_Transmit_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length);
uint8_t I2C_Slave_Receive_IT(I2C_Handle_t*i2c_handle,uint8_t *data,uint16_t length);
void I2C_IRQ_Handle(I2C_Handle_t*i2c_handle);

#endif /* INC_I2C_H_ */
