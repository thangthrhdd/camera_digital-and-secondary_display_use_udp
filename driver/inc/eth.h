/*
 * eth.h
 *
 *  Created on: Apr 5, 2026
 *      Author: ADMIN
 */

#ifndef INC_ETH_H_
#define INC_ETH_H_
#include "stm32f407.h"
///////////smi and mrii
typedef struct {
uint32_t Status; // Word 0 (TDES0 hoặc RDES0)
uint32_t ControlBufferSize; // Word 1
uint32_t Buffer1Addr; // Word 2
uint32_t Buffer2NextDescAddr; // Word 3
}ETH_DMADescTypeDef;

typedef struct
{
 uint8_t mode;
 uint8_t phy_addr;
 uint64_t mac_addr;
 uint32_t ip_addr;
}ETH_ConFig_t;
//mode

#define ETH_RMII_MODE 1
#define ETH_MII_MODE 0
#define IDLE 0
#define RUN 1
#define SUPSPEND 2
#define COMPLE 3
typedef struct
{
	//uint8_t Preamble[7];
	//uint8_t SFD;
	uint8_t Destination_addres[6];
	uint8_t Source_address[6];
	uint8_t MAC_client_length_type[2];
	uint8_t data[1498];
	//uint8_t Frame_check_sequence[4];
}__attribute__((aligned(4)))ETH_TRANS_DATA;

typedef struct
{
	uint8_t des_mac[6];
	uint8_t source_mac[6];
	uint16_t eth_type;
} __attribute__((packed))ETH_HEADER;
typedef struct
{

	uint8_t Version;
	uint8_t DSCP_ECN;
	uint16_t total_length;
	uint16_t Identification;
	uint16_t flag;
	uint8_t TTL;
	uint8_t protocol;
	uint16_t header_checksum;
	uint32_t source_ip;
	uint32_t des_ip;
} __attribute__((packed))ETH_IP_header;

typedef struct
{
	uint8_t Destination_addres[6];
	uint8_t Source_address[6];
	uint8_t MAC_client_length_type[2];
	union
	{
	ETH_IP_header iphead;
	uint8_t data[1498];
	};
	uint8_t crc[4];
}__attribute__((aligned(4)))ETH_REI_DATA;

typedef struct
{
	ETH_MAC_RegDef_t*eth_mac;
	ETH_MMC_RegDef_t*eth_mmc;
	ETH_IEEE_1588*eth_ieee;
	ETH_DMA_RegDef_t*eth_dma;
	ETH_ConFig_t eth_x;
	ETH_DMADescTypeDef*TX_BUFFER;
	ETH_DMADescTypeDef*RX_BUFFER;
	volatile uint8_t tx_state;
	volatile uint8_t rx_state;
	volatile ETH_REI_DATA*RX;
	volatile ETH_TRANS_DATA*TX;
	volatile uint16_t rx_length;
	char payload[256];
}ETH_Handle_t;




void ETH_CLK(ETH_Handle_t*eth_handle,uint8_t enordi);
void ETH_Init(ETH_Handle_t*eth_handle);
void ETH_TX_TRANS(ETH_Handle_t*eth_handle,uint8_t * packet,uint16_t length);
uint8_t ETH_RX_TRANS(ETH_Handle_t*eth_handle,uint8_t ** packet,uint16_t  *length);
void ETH_IRQ_Handle(ETH_Handle_t*eth_handle);
void ETH_RX_IRQ(ETH_Handle_t*eth_handle,uint8_t ** packet,uint16_t*length);
void ETH_TYPE(ETH_Handle_t *eth_handle);
#endif /* INC_ETH_H_ */
