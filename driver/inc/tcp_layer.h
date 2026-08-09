/*
 * tcp_layer.h
 *
 * Created on: Jun 16, 2026
 * Author: ADMIN
 */

#ifndef INC_TCP_LAYER_H_
#define INC_TCP_LAYER_H_

#include "stdint.h"
#include "eth.h"

// Quản lý trạng thái TCP toàn cục
typedef enum {
    TCP_LISTEN,
    TCP_SYN_SENT,       // MỚI: Trạng thái dành cho Client sau khi phát gói SYN
    TCP_SYN_RECEIVED,   // Dành cho Server sau khi nhận SYN và gửi SYN-ACK
    TCP_ESTABLISHED,
    TCP_CLOSED
} TCP_State_t;

// Cấu trúc chứa toàn bộ "danh tính mạng" của 1 kết nối
typedef struct {
    uint8_t  remote_mac[6];
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t seq_num;
    uint32_t ack_num;
    TCP_State_t state;
} TCP_Socket_t;

typedef struct
{
	uint16_t Source_Port;
	uint16_t Des_Port;
	uint32_t Seq_Number;
	uint32_t Ack_Number;
	uint8_t DOffset_Reserved;
	uint8_t TCP_Flags;
	uint16_t Window_Size;
	uint16_t TCP_Checksum;
	uint16_t Urgent_Pointer;
} __attribute__((packed))TCP_HEADER;

typedef struct
{
	ETH_HEADER eth_head;
	ETH_IP_header ip_header;
	TCP_HEADER tcp_header;
} __attribute__((packed))TCP_PACK_HEADER;

typedef struct
{
	TCP_State_t state;
	union
	{
	TCP_PACK_HEADER TCP_HEAD;
	uint8_t buffer[1512];
	};
} __attribute__((packed))TCP_PACK_SENDER;

#define MAX_TCP_SOCKETS 4

typedef struct {
    TCP_PACK_SENDER header;
    uint8_t         is_active;  // 1: Đang dùng, 0: Trống
    uint16_t        local_port;
} TCP_Socket_Manager_t;

extern TCP_Socket_Manager_t tcp_slots[MAX_TCP_SOCKETS];

void TCP_Layer_Init(void);
// MỚI: Hàm tìm socket thông minh sử dụng cả Port nguồn (Big Endian) và Port đích (Little Endian)
TCP_Socket_Manager_t* Get_Socket(uint16_t src_port_be, uint16_t dst_port_le);
void TCP_Send(ETH_Handle_t *eth_handle, TCP_PACK_SENDER *sock, uint8_t *payload, uint16_t payload_len, uint8_t flags);
void TCP_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *ip_pkt, uint16_t ip_total_len);

#endif /* INC_TCP_LAYER_H_ */
