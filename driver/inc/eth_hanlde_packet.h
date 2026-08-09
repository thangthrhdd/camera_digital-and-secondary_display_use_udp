/*
 * eth_hanlde_packet.h
 *
 *  Created on: Jun 11, 2026
 *      Author: ADMIN
 */

#ifndef INC_ETH_HANLDE_PACKET_H_
#define INC_ETH_HANLDE_PACKET_H_
#include "stm32f407.h"
#include "eth.h"
#include "gpio.h"

// --- EtherType ở lớp Ethernet ---
#define ETH_TYPE_IPV4        0x0800

// --- Giao thức ở lớp IP (Protocol) ---
#define IP_PROTO_TCP         0x06
#define IP_PROTO_UDP         0x11
#define IP_PROTO_ICMP        0x01

// --- Mặt nạ các cờ TCP (TCP Flags) ---
#define TCP_FLAG_FIN         0x01  // Ngắt kết nối
#define TCP_FLAG_SYN         0x02  // Khởi tạo kết nối (Handshake)
#define TCP_FLAG_RST         0x04  // Đóng kết nối lập tức (Reset)
#define TCP_FLAG_PSH         0x08  // Đẩy dữ liệu lên tầng ứng dụng ngay
#define TCP_FLAG_ACK         0x10  // Xác nhận đã nhận dữ liệu
#define TCP_FLAG_URG         0x20  // Dữ liệu khẩn cấp


typedef enum
{
	REPLY_TYPE_ARP,
	//REPLY_TYPE_TCP,
	REPLY_TYPE_ICMP_PING

}PACKET_ETH;
typedef enum
{
	REPLY_TYPE_TCP_SYN,
	REPLY_TYPE_TCP_SYN_ACK,
	REPLY_TYPE_TCP_ACK,
	REPLY_TYPE_TCP_PSH_ACK,
	REPLY_TYPE_FIN_ACK,
	REPLY_TYPE_RST,
}TCP_TYPE;
typedef struct {
    uint8_t  dest_mac[6];
    uint8_t  src_mac[6];
    uint16_t eth_type;
} __attribute__((packed))Eth_Header_t;

// Struct ARP Payload Layer 2.5 (28 Byte)
typedef struct {
    uint16_t htype;          // Loại phần cứng (Ethernet = 1)
    uint16_t ptype;          // Giao thức lớp trên (IPv4 = 0x0800)
    uint8_t  hlen;           // Độ dài MAC = 6
    uint8_t  plen;           // Độ dài IP = 4
    uint16_t op;             // Mã lệnh (1 = Req, 2 = Reply)
    uint8_t  sender_mac[6];  // MAC người gửi
    uint32_t sender_ip;      // IP người gửi
    uint8_t  target_mac[6];  // MAC người nhận
    uint32_t target_ip;      // IP người nhận
} __attribute__((packed))ARP_Payload_t;

typedef struct {
    Eth_Header_t  eth;
    ARP_Payload_t arp;
} __attribute__((packed))ARP_Packet_t;


///////////////////////////////////
typedef struct
{
	PACKET_ETH packet_type;
	union{
		ARP_Packet_t arp_packet;
		uint8_t tx_buffer[1512];
	};
}PACKET_REPLY;

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
void Process_HTTP_Request(ETH_Handle_t *eth_handle);
void Data_payload_handle(ETH_Handle_t *eth_handle);
void ETH_PACKET_REPLY(ETH_Handle_t *eth_handle, uint64_t my_mac, uint32_t my_ip,PACKET_REPLY * packet_rep,uint16_t length );
#endif /* INC_ETH_HANLDE_PACKET_H_ */
