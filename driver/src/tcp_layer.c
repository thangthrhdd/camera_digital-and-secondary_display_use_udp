/*
 * tcp_layer.c
 *
 * Created on: Jun 16, 2026
 * Author: ADMIN
 */
#include "tcp_layer.h"
#include "eth.h"
#include "mqtt.h"
#include <string.h>

TCP_Socket_Manager_t tcp_slots[4] = {0};

void TCP_Layer_Init(void) {
    memset(tcp_slots, 0, sizeof(tcp_slots));
    // Mặc định luôn để một slot ở trạng thái LISTEN cho cổng 80
    tcp_slots[0].is_active = 1;
    tcp_slots[0].local_port = 80;
    tcp_slots[0].header.state = TCP_LISTEN;
}

void TCP_Send(ETH_Handle_t *eth_handle, TCP_PACK_SENDER *sock, uint8_t *payload, uint16_t payload_len, uint8_t flags)
{
    uint16_t tcp_len = sizeof(TCP_HEADER) + payload_len;
    uint16_t ip_len  = sizeof(ETH_IP_header) + tcp_len;
    uint16_t total_len = sizeof(TCP_PACK_HEADER) + payload_len;

    sock->TCP_HEAD.eth_head.source_mac[0]=(uint8_t)(eth_handle->eth_x.mac_addr >> 40);
    sock->TCP_HEAD.eth_head.source_mac[1]=(uint8_t)(eth_handle->eth_x.mac_addr >> 32);
    sock->TCP_HEAD.eth_head.source_mac[2]=(uint8_t)(eth_handle->eth_x.mac_addr >> 24);
    sock->TCP_HEAD.eth_head.source_mac[3]=(uint8_t)(eth_handle->eth_x.mac_addr >> 16);
    sock->TCP_HEAD.eth_head.source_mac[4]=(uint8_t)(eth_handle->eth_x.mac_addr >> 8);
    sock->TCP_HEAD.eth_head.source_mac[5]=(uint8_t)(eth_handle->eth_x.mac_addr);

    // 1. CẬP NHẬT CÁC THÔNG TIN ĐỘNG CỦA IP HEADER
    sock->TCP_HEAD.eth_head.eth_type = __builtin_bswap16(0x0800);
    sock->TCP_HEAD.ip_header.Version = 0x45;
    sock->TCP_HEAD.ip_header.DSCP_ECN = 0x00;
    sock->TCP_HEAD.ip_header.total_length = __builtin_bswap16(ip_len);
    sock->TCP_HEAD.ip_header.Identification = __builtin_bswap16(0x0000);
    sock->TCP_HEAD.ip_header.flag = __builtin_bswap16(0x4000);
    sock->TCP_HEAD.ip_header.TTL = 128;
    sock->TCP_HEAD.ip_header.protocol = 0x06;

    // Tính IP Checksum
    sock->TCP_HEAD.ip_header.header_checksum = 0;
    uint32_t ip_sum = 0;
    uint16_t *ip_ptr = (uint16_t*)&sock->TCP_HEAD.ip_header;
    for(int i = 0; i < (sizeof(ETH_IP_header) / 2); i++) {
        ip_sum += __builtin_bswap16(ip_ptr[i]);
    }
    while(ip_sum >> 16) ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);
    sock->TCP_HEAD.ip_header.header_checksum = __builtin_bswap16((uint16_t)(~ip_sum));

    // 2. CẬP NHẬT CÁC THÔNG TIN ĐỘNG CỦA TCP HEADER
    sock->TCP_HEAD.tcp_header.DOffset_Reserved = 0x50;
    sock->TCP_HEAD.tcp_header.TCP_Flags = flags;
    sock->TCP_HEAD.tcp_header.Window_Size = __builtin_bswap16(2048);
    sock->TCP_HEAD.tcp_header.Urgent_Pointer = 0;

    // 3. ĐẬP PAYLOAD VÀO VÙNG ĐỆM KẾ TIẾP CỦA UNION
    if(payload_len > 0 && payload != NULL) {
        memcpy(&sock->buffer[sizeof(TCP_PACK_HEADER)], payload, payload_len);
    }

    // 4. TÍNH TCP CHECKSUM (Bao gồm Pseudo Header)
    sock->TCP_HEAD.tcp_header.TCP_Checksum = 0;
    uint32_t tcp_sum = 0;

    uint32_t src_ip = __builtin_bswap32(sock->TCP_HEAD.ip_header.source_ip);
    uint32_t des_ip = __builtin_bswap32(sock->TCP_HEAD.ip_header.des_ip);
    tcp_sum += (src_ip >> 16) + (src_ip & 0xFFFF);
    tcp_sum += (des_ip >> 16) + (des_ip & 0xFFFF);
    tcp_sum += 0x0006 + tcp_len;

    int tcp_words = tcp_len / 2;
    uint16_t *tcp_ptr = (uint16_t*)&sock->TCP_HEAD.tcp_header;
    for(int i = 0; i < tcp_words; i++) {
        tcp_sum += __builtin_bswap16(tcp_ptr[i]);
    }
    if(tcp_len % 2 != 0) {
        tcp_sum += sock->buffer[sizeof(TCP_PACK_HEADER) + payload_len - 1] << 8;
    }
    while(tcp_sum >> 16) tcp_sum = (tcp_sum & 0xFFFF) + (tcp_sum >> 16);
    sock->TCP_HEAD.tcp_header.TCP_Checksum = __builtin_bswap16((uint16_t)(~tcp_sum));

    // 5. BẮN BUFFER RA NGOẠI VI DMA THÔ
    ETH_TX_TRANS(eth_handle, sock->buffer, total_len);

    // 6. TỰ ĐỘNG TĂNG SEQ_NUMBER PHÙ HỢP VỚI CỜ TRUYỀN ĐI
    uint32_t current_seq = __builtin_bswap32(sock->TCP_HEAD.tcp_header.Seq_Number);
    current_seq += payload_len;
    if (flags & 0x02) current_seq++; // Gói SYN tính là 1 đơn vị Seq
    if (flags & 0x01) current_seq++; // Gói FIN tính là 1 đơn vị Seq
    sock->TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(current_seq);
}

// SỬA BUG LỆCH ENDIAN: Đồng bộ toàn bộ về định dạng Host-Order (Little-Endian) để so sánh
TCP_Socket_Manager_t* Get_Socket(uint16_t remote_port_le, uint16_t local_port_le) {
    // Bước 1: Tìm kiếm socket động đang hoạt động
    for(int i = 0; i < MAX_TCP_SOCKETS; i++) {
        if(tcp_slots[i].is_active && tcp_slots[i].local_port == local_port_le && tcp_slots[i].header.state != TCP_LISTEN) {
            // Chuyển đổi Des_Port từ Big-Endian sang Little-Endian trước khi so sánh
            uint16_t slot_remote_port = __builtin_bswap16(tcp_slots[i].header.TCP_HEAD.tcp_header.Des_Port);
            if(slot_remote_port == remote_port_le) {
                return &tcp_slots[i];
            }
        }
    }
    // Bước 2: Tìm kiếm socket Server đang ở trạng thái LISTEN
    for(int i = 0; i < MAX_TCP_SOCKETS; i++) {
        if(tcp_slots[i].is_active && tcp_slots[i].local_port == local_port_le && tcp_slots[i].header.state == TCP_LISTEN) {
            return &tcp_slots[i];
        }
    }
    return NULL;
}

// HÀM TIẾP NHẬN XỬ LÝ GÓI TCP ĐÃ ĐƯỢC VÁ TOÀN DIỆN
void TCP_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *ip_pkt, uint16_t ip_total_len)
{
    uint8_t ip_hlen = (ip_pkt[0] & 0x0F) * 4;
    uint8_t *tcp_pkt = &ip_pkt[ip_hlen];
    uint8_t tcp_hlen = ((tcp_pkt[12] >> 4) & 0x0F) * 4;

    // --- SỬA BUG 3: Trích xuất độ dài thực tế từ IP Header để triệt tiêu Ethernet Padding ---
    uint16_t real_ip_total_len = ((uint16_t)ip_pkt[2] << 8) | ip_pkt[3];
    uint16_t payload_len = real_ip_total_len - ip_hlen - tcp_hlen;

    // Tính toán Port theo Host-Order (Little-Endian) chuẩn xác
    uint16_t remote_port = ((uint16_t)tcp_pkt[0] << 8) | tcp_pkt[1];
    uint16_t local_port  = ((uint16_t)tcp_pkt[2] << 8) | tcp_pkt[3];

    TCP_Socket_Manager_t *sock = Get_Socket(remote_port, local_port);
    if (!sock) return;

    // Cập nhật thông tin định tuyến phản hồi vào Template của Socket
    memcpy(&sock->header.TCP_HEAD.eth_head.des_mac, eth_handle->RX->Source_address, 6);
    memcpy(&sock->header.TCP_HEAD.ip_header.des_ip, &ip_pkt[12], 4);
    memcpy(&sock->header.TCP_HEAD.ip_header.source_ip, &ip_pkt[16], 4);
    memcpy(&sock->header.TCP_HEAD.tcp_header.Des_Port, &tcp_pkt[0], 2);
    memcpy(&sock->header.TCP_HEAD.tcp_header.Source_Port, &tcp_pkt[2], 2);

    // Chuyển đổi Seq/Ack nhận được sang Little-Endian để xử lý toán học mạng
    uint32_t rx_seq = ((uint32_t)tcp_pkt[4] << 24) | ((uint32_t)tcp_pkt[5] << 16) |
                      ((uint32_t)tcp_pkt[6] << 8)  | tcp_pkt[7];
    uint32_t rx_ack = ((uint32_t)tcp_pkt[8] << 24) | ((uint32_t)tcp_pkt[9] << 16) |
                      ((uint32_t)tcp_pkt[10] << 8) | tcp_pkt[11];
    uint8_t flags = tcp_pkt[13];

    switch (sock->header.state)
    {
        case TCP_LISTEN:
            if (flags & 0x02) {
                sock->header.TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(0x11223344);
                sock->header.TCP_HEAD.tcp_header.Ack_Number = __builtin_bswap32(rx_seq + 1);

                TCP_Send(eth_handle, &(sock->header), NULL, 0, 0x12); // Gửi SYN-ACK
                sock->header.state = TCP_SYN_RECEIVED;
            }
            break;

        case TCP_SYN_SENT:
            if ((flags & 0x12) == 0x12) { // Nhận đúng cờ SYN+ACK
                sock->header.TCP_HEAD.tcp_header.Ack_Number = __builtin_bswap32(rx_seq + 1);
                sock->header.TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(rx_ack);

                sock->header.state = TCP_ESTABLISHED;
                TCP_Send(eth_handle, &(sock->header), NULL, 0, 0x10); // Gửi ACK chốt bắt tay

                uint16_t r_port = __builtin_bswap16(sock->header.TCP_HEAD.tcp_header.Des_Port);
                if (r_port == 1883 || sock->local_port == 18830) {
                    MQTT_Process_Rx(eth_handle, NULL, 0); // Kích hoạt MQTT gửi CONNECT
                }
            }
            break;

        case TCP_SYN_RECEIVED:
            if ((flags & 0x10) && !(flags & 0x02)) {
                sock->header.state = TCP_ESTABLISHED;
            }
            break;

        case TCP_ESTABLISHED:
            // Đồng bộ lại Seq phát kế tiếp khớp với Ack bên kia mong muốn
            sock->header.TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(rx_ack);

            uint32_t next_ack = rx_seq + payload_len;
            if (flags & 0x01) next_ack++; // Khử cờ FIN nếu đối phương muốn đóng kết nối
            sock->header.TCP_HEAD.tcp_header.Ack_Number = __builtin_bswap32(next_ack);

            // Xử lý khi nhận được dữ liệu Payload ứng dụng
            if (payload_len > 0) {
                uint8_t *tcp_payload = &tcp_pkt[tcp_hlen];
                uint16_t r_port = __builtin_bswap16(sock->header.TCP_HEAD.tcp_header.Des_Port);

                if (sock->local_port == 80) {
                    memcpy((uint8_t*)eth_handle->payload, tcp_payload, payload_len);
                    eth_handle->payload[payload_len] = '\0';
                    Process_HTTP_Request(eth_handle, &sock->header);
                }
                else if (r_port == 1883 || sock->local_port == 18830) {
                    MQTT_Process_Rx(eth_handle, tcp_payload, payload_len);

                    // --- SỬA BUG 2: BẮT BUỘC PHẢI GỬI ACK TRẢ VỀ CHO BROKER ---
                    // MQTT không tự sinh gói phản hồi ngay lập tức như HTTP, nên cần phản hồi ACK thuần
                    TCP_Send(eth_handle, &sock->header, NULL, 0, 0x10);
                }
            }
            // Nhận cờ đóng kết nối FIN từ đối phương
             if (flags & 0x01) {
                TCP_Send(eth_handle, &sock->header, NULL, 0, 0x11);
                if (sock->local_port == 80) {
                    sock->header.state = TCP_LISTEN;
                } else {
                    sock->header.state = TCP_CLOSED;
                }
            }
            break;

        default:
            break;
    }
}
