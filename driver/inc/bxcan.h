/*
 * bxcan.h
 *
 *  Created on: Jun 29, 2026
 *      Author: ADMIN
 */

#ifndef INC_BXCAN_H_
#define INC_BXCAN_H_
#include "stm32f407.h"
typedef enum
{
	CAN_STATE_NORMAL,
	CAN_STATE_INIT,
	CAN_STATE_SLEEP
}CAN_State_t;
typedef enum
{
	CAN_TX_STATE_EMPTY,
	CAN_TX_STATE_PENDING,
	CAN_TX_STATE_SCHEDULED,
	CAN_TX_STATE_TRANSMIT,
	CAN_TX_STATE_ERROR

}CAN_TX_State_t;
typedef enum
{
	CAN_RX_STATE_EMPTY,
	CAN_RX_STATE_PENDING_1,
	CAN_RX_STATE_PENDING_2,
	CAN_RX_STATE_PENDING_3,
	CAN_RX_STATE_OVERRUN
}CAN_RX_State_t;
#define NORMAL_MODE 0
#define DEBUG_MODE 2
#define LOOPBACK_MODE 1


#define CAN_BAUD_250kpb 250000
#define CAN_BAUD_500kbp 500000
#define CAN_BAUD_1mbp 1000000
typedef struct
{
	uint8_t can_mode;
	uint8_t auto_retransmission;
	uint32_t baudrate;


}CAN_TypeDef_t;
typedef struct
{
	uint8_t channel;
	uint8_t Filter_mode;
	uint8_t Filter_scale;
	uint8_t assigment_fifo;
	uint8_t active;
	uint32_t filter_id1;
	uint32_t filter_id2;
}CAN_Filter_config;
typedef struct
{
	uint32_t id;
	uint8_t length;
	uint8_t data[8];
}Data_packet;
typedef struct
{
	CAN_RegDef_t * canx;
	CAN_TypeDef_t cany;
	CAN_State_t state;
	CAN_TX_State_t tx_state[3];
	CAN_RX_State_t rx_state[2];
	Data_packet rx_packet[2][3];
}CAN_Handle_t;

void CAN_CLK(CAN_RegDef_t * canx,uint8_t EnorDi);
void CAN_Init(CAN_Handle_t *can_handle);
void CAN_Filter_Init(CAN_Handle_t *can_handle,CAN_Filter_config canz);
void CAN_TX_Transmit(CAN_Handle_t *can_handle,uint16_t id,uint8_t  data[8],uint16_t length);
void CAN_RX_Transmit(CAN_Handle_t *can_handle,uint8_t fifo);
void CAN_IRQ_RXFF1_Handle(CAN_Handle_t* can_handle);
void CAN_IRQ_RXFF0_Handle(CAN_Handle_t* can_handle);
void CAN_IRQ_TX_Handle(CAN_Handle_t* can_handle);
#endif /* INC_BXCAN_H_ */
