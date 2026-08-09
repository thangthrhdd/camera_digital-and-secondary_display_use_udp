/*
 * uart.h
 *
 *  Created on: Nov 7, 2025
 *      Author: ADMIN
 */

#ifndef INC_UART_H_
#define INC_UART_H_
#include"stm32f407.h"
typedef struct
{
	uint8_t mode_device;
	uint8_t stop_bit;
	uint8_t parity_cs;
	uint8_t word_length;
	uint32_t baud_rate;
	uint8_t hardware_flow_control;
	uint8_t over_sample;

}UART_ConFig_t;
typedef struct
{
	UART_RegDef_t* uartx;
	UART_ConFig_t uarty;
	//irq
	uint8_t *tx_data;
	uint8_t* rx_data;
	uint8_t state;
	uint16_t tx_length;
	uint16_t rx_length;

}UART_Handle_t;
//mode
#define UART_TX_ONLY 2
#define UART_RX_ONLY 1
#define UART_RXTX 3
//stop-bit
#define UART_STOP_1BIT 0
#define UART_STOP_0_5BIT 1
#define UART_STOP_2BIT 2
#define UART_STOP_1_5BIT 3
//parity_cs
#define UART_PARITY_DI 0
#define UART_PARITY_ODD 3
#define UART_PARITY_EVEN 2
//word length
#define UART_8BIT 0
#define UART_9BIT 1
//baud Rate
#define UART_BAUD_1200 1200
#define UART_BAUD_2400 2400
#define UART_BAUD_9600 9600
#define UART_BAUD_19000 19200
#define UART_BAUD_38000 38400
#define UART_BAUD_57000 57600
#define UART_BAUD_115000 115200

// hardware flow control
#define UART_HW_DI 0
#define UART_HW_RTSE 1
#define UART_HW_CTSE 2
#define UART_HW_RTCT 3
//over ssamp
#define UART_OVERSAMP_8 1
#define UART_OVERSAMP_16 0
//IRQ state
#define UART_IDLE 0
#define UART_BSY_TX 1
#define UART_BSY_RX 2
#define UART_CMPL 3

void UART_CLK(UART_RegDef_t* uartx,uint8_t EnorDi);
void UART_Init(UART_Handle_t*uart_handle);
void UART_Control(UART_RegDef_t* uartx,uint8_t EnorDi);
void UART_Transmit(UART_RegDef_t* uartx,uint8_t*data,uint16_t length);
void UART_Receive(UART_RegDef_t* uartx,uint8_t*data,uint16_t length);

uint8_t UART_Transmit_IT(UART_Handle_t*uart_handle,uint8_t*data,uint16_t length);
uint8_t UART_Receive_IT(UART_Handle_t*uart_handle,uint8_t*data,uint16_t length);
void UART_IRQ_Handle(UART_Handle_t*uart_handle);


#endif /* INC_UART_H_ */
