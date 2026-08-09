/*
 * udp_layer.c
 *
 *  Created on: Jun 16, 2026
 *      Author: ADMIN
 */
#include "udp_layer.h" // Cần nhúng file dns.h để lấy hằng số DNS_CLIENT_PORT (55553)
#include <string.h>
#include"tftcolor.h"
#include "spi.h"
#include "dma.h"
static uint16_t Calculate_UDP_Checksum(uint32_t src_ip, uint32_t dst_ip, UDP_HEADER *udp_header, uint8_t *payload, uint16_t payload_len)
{
    uint32_t sum = 0;
    uint16_t udp_len = sizeof(UDP_HEADER) + payload_len;

    // 1. Pseudo Header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += (src_ip) & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += (dst_ip) & 0xFFFF;
    sum += 0x0011; // Protocol UDP (17)
    sum += udp_len;

    // 2. UDP Header
    uint16_t *ptr = (uint16_t *)udp_header;
    for (int i = 0; i < 4; i++) { // UDP header có 4 trường 2-byte
        sum += ptr[i];
    }

    // 3. Payload
    uint16_t *p_payload = (uint16_t *)payload;
    for (int i = 0; i < payload_len / 2; i++) {
        sum += p_payload[i];
    }

    // Nếu payload lẻ byte
    if (payload_len % 2 != 0) {
        sum += (uint16_t)(payload[payload_len - 1] << 8);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    uint16_t result = (uint16_t)(~sum);
    return (result == 0) ? 0xFFFF : result;
}
volatile uint8_t *udp_data;
volatile uint16_t payload_len;
extern SPI_Handle_t spi1;
extern volatile uint8_t frame_finished;
static volatile TFT_Line_RingBuffer_t line_ring;
extern volatile TFT_Line_Element_t active_line;
void TFT_RingBuffer_Init(void) {
    line_ring.head = 0;
    line_ring.tail = 0;
}

// Hàm đẩy dữ liệu vào bộ đệm vòng trong ngắt Ethernet
void TFT_RingBuffer_Push(uint16_t id, uint8_t* data) {
    uint16_t next_tail = (line_ring.tail + 1) % LINE_RING_BUFFER_SIZE;

    // Nếu đầy bộ đệm vòng, bỏ qua gói cũ để tránh làm treo hệ thống nhúng
    if (next_tail == line_ring.head) {
        return;
    }

    line_ring.storage[line_ring.tail].line_id = id;
    // Copy nhanh 1280 bytes dữ liệu màu vào ô đệm trống
    memcpy(line_ring.storage[line_ring.tail].color_data, data, 1280);

    line_ring.tail = next_tail;
}

// Hàm bốc dữ liệu ra xử lý ở Main
uint8_t TFT_RingBuffer_Pop(TFT_Line_Element_t *out_element) {
     if (line_ring.head == line_ring.tail) {
        return 0; // Bộ đệm rỗng
    }

    *out_element = line_ring.storage[line_ring.head];
    line_ring.head = (line_ring.head + 1) % LINE_RING_BUFFER_SIZE;
    return 1;
}
void UDP_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *ip_pkt, uint16_t ip_total_len)
{
    // Ánh xạ buffer nhận được vào struct IP header để lấy thông tin
    ETH_IP_header *ip_head = (ETH_IP_header *)ip_pkt;
    uint8_t ip_header_len = (ip_head->Version & 0x0F) * 4;

    // Trỏ vào UDP header bắt đầu ngay sau IP header
    UDP_HEADER *udp_head = (UDP_HEADER *)(ip_pkt + ip_header_len);

    // Đảo byte để chuyển từ Network Byte Order sang Host Byte Order
    uint16_t dest_port = __builtin_bswap16(udp_head->Des_port);
    uint16_t udp_len   = __builtin_bswap16(udp_head->Length);

    // Payload nằm ngay sau 8 byte UDP header
    uint8_t *udp_payload = (uint8_t *)(udp_head + 1);
    udp_data=udp_payload;
    uint16_t udp_payload_len = udp_len - 8;
    payload_len=udp_payload_len;
/*	if (udp_data[0] == 't' && udp_data[1] == 'f' && udp_data[2] == 't')
	    {
	        // 1. Parse ID an toàn trên STM32 (Tránh Unaligned RAM Access)
	        uint16_t block_id = udp_data[3] | (udp_data[4] << 8);

	        // 2. Tính toán tọa độ X, Y (Áp dụng cho block 4x4)
	        uint16_t block_x = block_id % 80;
	        uint16_t block_y = block_id / 80;

	        uint16_t pixel_x = block_x * 4;
	        uint16_t pixel_y = block_y * 4;

	       /* // 3. Set vùng vẽ và đổ DMA (Giữ nguyên đoạn code DMA cực chuẩn lần trước)
	        tft_set_window(&spi1, pixel_x, pixel_y, pixel_x + 3, pixel_y + 3);

	        TFT_DC_DATA();
	        TFT_CS_LOW();

	        DMA2->DMA_MEM[3].SCR &= ~(1 << 0);
	        while(DMA2->DMA_MEM[3].SCR & (1 << 0));
	        DMA2->LIFCR |= (0x3D << 22);

	        DMA2->DMA_MEM[3].SM0AR = (uint32_t)&udp_data[5];
	        DMA2->DMA_MEM[3].SNDTR = 32;
	        DMA2->DMA_MEM[3].SCR |= (1 << 0);

	        // Chờ DMA xong thì nhả CS
	        while ( !(DMA2->LISR & (1 << 27)) );
	        DMA2->LIFCR |= (1 << 27);
	        TFT_CS_HIGH();
	        //tft_draw_image(&spi1, pixel_x, pixel_y,4,4, &udp_data[5]);
	    }*/
    // Kiểm tra định dạng gói tin dải dòng lớn: 3 byte đầu 't','f','t' và payload dài đúng 1285 bytes
    // Kiểm tra Header 2 ký tự 'T' và 'F', cùng độ dài gói mới là 1284
    if (payload_len == 1284 && udp_payload[0] == 't' && udp_payload[1] == 'f')
    {
        // Lấy Line ID từ byte số 2 và 3
        uint16_t line_id = (uint16_t)(udp_payload[2] | (udp_payload[3] << 8));

        // Dữ liệu màu bắt đầu từ byte số 4 (Đẩy thẳng vào Ring Buffer hoặc SPI DMA từ index này)
        TFT_RingBuffer_Push(line_id, &udp_payload[4]);
    }

    if (dest_port == DNS_CLIENT_PORT) {
        DNS_Process_Rx(eth_handle, udp_payload, udp_payload_len);
    }
}
void UDP_Send(ETH_Handle_t *eth_handle, uint8_t *remote_mac, uint32_t remote_ip,
              uint16_t local_port, uint16_t remote_port, uint8_t *payload, uint16_t payload_len)
{
    static UDP_SEND_PACKET packet;
    memset(packet.tx_buff, 0, sizeof(packet.tx_buff));

    uint16_t udp_len   = 8 + payload_len;
    uint16_t ip_len    = 20 + udp_len;

    // 1. Ethernet Header
    memcpy(packet.udp_packet.eth_head.des_mac, remote_mac, 6);
    packet.udp_packet.eth_head.source_mac[0] = (uint8_t)(eth_handle->eth_x.mac_addr >> 40);
    packet.udp_packet.eth_head.source_mac[1] = (uint8_t)(eth_handle->eth_x.mac_addr >> 32);
    packet.udp_packet.eth_head.source_mac[2] = (uint8_t)(eth_handle->eth_x.mac_addr >> 24);
    packet.udp_packet.eth_head.source_mac[3] = (uint8_t)(eth_handle->eth_x.mac_addr >> 16);
    packet.udp_packet.eth_head.source_mac[4] = (uint8_t)(eth_handle->eth_x.mac_addr >> 8);
    packet.udp_packet.eth_head.source_mac[5] = (uint8_t)(eth_handle->eth_x.mac_addr);
    packet.udp_packet.eth_head.eth_type = __builtin_bswap16(0x0800); // 0x0800 cho IPv4

    // 2. IP Header
    packet.udp_packet.ip_head.Version = 0x45;
    packet.udp_packet.ip_head.total_length = __builtin_bswap16(ip_len);
    packet.udp_packet.ip_head.Identification = __builtin_bswap16(0x0001);
    packet.udp_packet.ip_head.flag = 0; // Flags = 0
    packet.udp_packet.ip_head.TTL = 64;
    packet.udp_packet.ip_head.protocol = 17; // UDP
    packet.udp_packet.ip_head.source_ip = __builtin_bswap32(eth_handle->eth_x.ip_addr);
    packet.udp_packet.ip_head.des_ip = __builtin_bswap32(remote_ip);
    packet.udp_packet.ip_head.header_checksum = 0;

    // 3. Tính IP Checksum (Chuẩn RFC 791)
    uint32_t ip_sum = 0;
    uint8_t *ip_ptr = (uint8_t *)&packet.udp_packet.ip_head;
    for (int i = 0; i < 10; i++) {
        ip_sum += (uint16_t)((ip_ptr[i * 2] << 8) | ip_ptr[i * 2 + 1]);
    }
    while (ip_sum >> 16) {
        ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);
    }
    packet.udp_packet.ip_head.header_checksum = __builtin_bswap16((uint16_t)(~ip_sum));

    // 4. UDP Header
    packet.udp_packet.udp_head.Source_port = __builtin_bswap16(local_port);
    packet.udp_packet.udp_head.Des_port = __builtin_bswap16(remote_port);
    packet.udp_packet.udp_head.Length = __builtin_bswap16(udp_len);
    packet.udp_packet.udp_head.Checksum = 0; // Tạm thời để 0 để tính toán

    // ========================================================================
    // ĐẶC BIỆT: Phải copy payload vào buffer TRƯỚC KHI tính UDP Checksum
    // ========================================================================
    memcpy(packet.tx_buff + sizeof(UDP_PACKET), payload, payload_len);

    // 5. Tính UDP Checksum (Chuẩn RFC 768)
    uint32_t udp_sum = 0;

    // 5.1. Cộng Pseudo-Header (Source IP, Dest IP, Protocol, UDP Length)
    uint8_t *src_ip = (uint8_t *)&packet.udp_packet.ip_head.source_ip;
    uint8_t *dst_ip = (uint8_t *)&packet.udp_packet.ip_head.des_ip;

    udp_sum += (uint16_t)((src_ip[0] << 8) | src_ip[1]);
    udp_sum += (uint16_t)((src_ip[2] << 8) | src_ip[3]);
    udp_sum += (uint16_t)((dst_ip[0] << 8) | dst_ip[1]);
    udp_sum += (uint16_t)((dst_ip[2] << 8) | dst_ip[3]);
    udp_sum += 0x0011;       // Protocol = 17 (UDP)
    udp_sum += udp_len;      // Chiều dài thực tế của gói UDP

    // 5.2. Cộng UDP Header và UDP Payload
    uint8_t *udp_ptr = (uint8_t *)&packet.udp_packet.udp_head;
    for (int i = 0; i < udp_len / 2; i++) {
        udp_sum += (uint16_t)((udp_ptr[i * 2] << 8) | udp_ptr[i * 2 + 1]);
    }

    // 5.3. Xử lý trường hợp chiều dài gói tin là số lẻ (Padding thêm byte 0)
    if (udp_len % 2 != 0) {
        udp_sum += (uint16_t)(udp_ptr[udp_len - 1] << 8);
    }

    // 5.4. Gộp các bit bị tràn (Fold 32-bit to 16-bit)
    while (udp_sum >> 16) {
        udp_sum = (udp_sum & 0xFFFF) + (udp_sum >> 16);
    }

    // 5.5. Đảo bit và lưu vào header
    uint16_t final_udp_checksum = (uint16_t)(~udp_sum);

    // Theo chuẩn UDP, nếu Checksum tính ra là 0x0000 thì phải đổi thành 0xFFFF
    if (final_udp_checksum == 0x0000) {
        final_udp_checksum = 0xFFFF;
    }
    packet.udp_packet.udp_head.Checksum = __builtin_bswap16(final_udp_checksum);
    // ========================================================================

    // 6. Padding frame đạt chuẩn Ethernet (Tối thiểu 64 bytes)
    uint16_t total_frame_len = sizeof(UDP_PACKET) + payload_len;
    if (total_frame_len < 64) {
        total_frame_len = 64;
    }

    // 7. Gọi hàm truyền
    ETH_TX_TRANS(eth_handle, packet.tx_buff, total_frame_len);
}
// Thêm vào udp_layer.c
uint8_t TFT_RingBuffer_IsFull(void) {
    uint16_t next_tail = (line_ring.tail + 1) % LINE_RING_BUFFER_SIZE;
    return (next_tail == line_ring.head);
}
