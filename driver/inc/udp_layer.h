/*
 * udp_layer.h
 *
 *  Created on: Jun 16, 2026
 *      Author: ADMIN
 */

#ifndef INC_UDP_LAYER_H_
#define INC_UDP_LAYER_H_
#include <stdint.h>
#include "dns.h"
#include "eth.h" // Nhúng để có cấu trúc ETH_Handle_t


typedef struct
{
	uint16_t Source_port;
	uint16_t Des_port;
	uint16_t Length;
	uint16_t Checksum;
} __attribute__((packed))UDP_HEADER;

typedef struct
{
	ETH_HEADER eth_head;
	ETH_IP_header ip_head;
	UDP_HEADER udp_head;
} __attribute__((packed)) UDP_PACKET;

typedef struct
{
	union
	{
	UDP_PACKET udp_packet;
	uint8_t tx_buff[1512];
	}
} __attribute__((packed)) UDP_SEND_PACKET;
/**x
 * @brief Hàm xử lý gói tin UDP đi vào (Tầng Transport)
 * @param eth_handle: Con trỏ quản lý driver mạng
 * @param ip_pkt: Con trỏ trỏ thẳng vào vùng bắt đầu của IP Header
 * @param ip_total_len: Tổng chiều dài gói IP
 */
void UDP_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *ip_pkt, uint16_t ip_total_len);

/**
 * @brief Hàm gửi gói tin UDP vạn năng (Dùng cho DNS, DHCP...)
 */
void UDP_Send(ETH_Handle_t *eth_handle, uint8_t *remote_mac, uint32_t remote_ip,
              uint16_t local_port, uint16_t remote_port, uint8_t *payload, uint16_t payload_len);

// Cấu trúc dải dòng gom cụm
typedef struct {
    uint16_t line_id;
    uint8_t  color_data[1280]__attribute__((aligned(4))); // 320 pixel * 2 dòng * 2 bytes = 1280 bytes
} TFT_Line_Element_t;

// Mở rộng kích thước bộ đệm vòng lên 16 dải
#define LINE_RING_BUFFER_SIZE  65

typedef struct {
    TFT_Line_Element_t storage[LINE_RING_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
}TFT_Line_RingBuffer_t ;

void TFT_RingBuffer_Init(void);
void UDP_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *ip_pkt, uint16_t ip_total_len);
uint8_t TFT_RingBuffer_Pop(TFT_Line_Element_t *out_element);
void TFT_RingBuffer_Push(uint16_t id, uint8_t* data);
// Thêm vào cuối file udp_layer.h
uint8_t TFT_RingBuffer_IsFull(void);
#endif /* INC_UDP_LAYER_H_ */
