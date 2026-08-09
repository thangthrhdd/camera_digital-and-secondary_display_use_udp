
#include "eth_hanlde_packet.h"
// 1. SỬA HÀM NHẬN: Phải dịch con trỏ payload vào sau các Header
/*void Data_payload_handle(ETH_Handle_t *eth_handle)
{
    const uint8_t HEADER_OFFSET = 54;

    if (eth_handle->rx_length > HEADER_OFFSET)
    {
        uint16_t payload_len = eth_handle->rx_length - HEADER_OFFSET;

        if (payload_len > 0)
        {
            // 1. Trích xuất Port Đích (Local Port của STM32) từ gói RX để phân loại dịch vụ
            uint8_t *rx_ip = eth_handle->RX->data;
            uint8_t rx_ip_hlen = (rx_ip[0] & 0x0F) * 4;
            uint8_t *rx_tcp = &rx_ip[rx_ip_hlen];

            // Port đích nằm ở byte thứ 2 và 3 của TCP Header
            uint16_t local_port = ((uint16_t)rx_tcp[2] << 8) | rx_tcp[3];

            // 2. Định tuyến dựa trên Cổng kết nối (Port)
            switch (local_port)
            {
                case 80: // PORT HTTP chuẩn
                    if (strstr((char*)eth_handle->payload, "GET") != NULL)
                    {
                        Process_HTTP_Request(eth_handle);
                    }
                    break;

                case 1883: // PORT MQTT chuẩn
                    // Process_MQTT_Request(eth_handle); // Sẵn sàng cho hàm xử lý MQTT của bạn tại đây!
                    break;

                default:
                    // Nhận vào port không hỗ trợ
                    break;
            }
        }
    }
}

// 2. SỬA HÀM XỬ LÝ LỆNH: Giữ nguyên logic của bạn nhưng dùng strstr cho chuẩn
// Giả định bạn có khai báo MAC và IP của mạch ở đâu đó (trong main.c)
extern uint64_t MY_MAC_ADDR;
extern uint32_t MY_IP_ADDR;

void Process_HTTP_Request(ETH_Handle_t *eth_handle)
{
    char *data = (char *)eth_handle->payload;

    if (strstr(data, "GET /on") != NULL)
    {
        GPIO_WritePin(GPIOC, GPIO_PIN_6,SET);

        // Gọi hàm gửi kèm MAC và IP của STM32
        ETH_SEND_HTTP_RESPONSE(eth_handle,eth_handle->eth_x.mac_addr,eth_handle->eth_x.ip_addr, "ON");
    }
    else if (strstr(data, "GET /off") != NULL)
    {
        GPIO_WritePin(GPIOC, GPIO_PIN_6,RESET);

        ETH_SEND_HTTP_RESPONSE(eth_handle,eth_handle->eth_x.mac_addr,eth_handle->eth_x.ip_addr, "OFF");
    }
    else if (strstr(data, "GET / HTTP") != NULL)
    {
        // Trình duyệt truy cập trang chủ mặc định (http://<IP_STM32>/)
        ETH_SEND_HTTP_RESPONSE(eth_handle,eth_handle->eth_x.mac_addr,eth_handle->eth_x.ip_addr, "HOME");
    }
    else
    {
        // Nếu trình duyệt tự động gửi lệnh lạ (ví dụ xin file favicon.ico)
        // Ta vẫn phản hồi trang chủ để tránh bị treo tab trình duyệt
        ETH_SEND_HTTP_RESPONSE(eth_handle, eth_handle->eth_x.mac_addr,eth_handle->eth_x.ip_addr, "HOME");
    }
}

void ETH_SEND_HTTP_RESPONSE(ETH_Handle_t *eth_handle, uint64_t my_mac, uint32_t my_ip, char *status)
{
    // Tăng kích thước buffer lên 1024 để chứa vừa toàn bộ trang HTML
    static uint8_t tx_buf[1024];
    memset(tx_buf, 0, sizeof(tx_buf));

    // =========================================================
    // 1. LẤY THÔNG TIN TỪ GÓI RX (Giữ nguyên logic parser của bạn)
    // =========================================================
    uint8_t *rx_ip = eth_handle->RX->data;
    uint8_t rx_ip_hlen = (rx_ip[0] & 0x0F) * 4;
    uint8_t *rx_tcp = &rx_ip[rx_ip_hlen];
    uint8_t rx_tcp_hlen = ((rx_tcp[12] >> 4) & 0x0F) * 4;

    uint16_t rx_ip_total_len = ((uint16_t)rx_ip[2] << 8) | (uint16_t)rx_ip[3];
    uint16_t tcp_payload_len = rx_ip_total_len - rx_ip_hlen - rx_tcp_hlen;
    uint8_t tcp_flags = rx_tcp[13];

    uint32_t rx_seq = ((uint32_t)rx_tcp[4] << 24) | ((uint32_t)rx_tcp[5] << 16) |
                      ((uint32_t)rx_tcp[6] << 8)  | (uint32_t)rx_tcp[7];
    uint32_t rx_ack = ((uint32_t)rx_tcp[8] << 24) | ((uint32_t)rx_tcp[9] << 16) |
                      ((uint32_t)rx_tcp[10] << 8) | (uint32_t)rx_tcp[11];

    uint32_t tx_seq = rx_ack;
    uint32_t tx_ack = rx_seq + tcp_payload_len;

    if(tcp_flags & 0x02) tx_ack++; // Khử cờ SYN
    if(tcp_flags & 0x01) tx_ack++; // Khử cờ FIN

    // =========================================================
    // 1b. CHÈN DỮ LIỆU HTTP VÀO PAYLOAD (Bắt đầu từ byte thứ 54)
    // =========================================================
    char html_body[256];
    int body_len = snprintf(html_body, sizeof(html_body),
        "<html><body>"
        "<h1>STM32 Web Server</h1>"
        "<p>Trang thai LED: <b>%s</b></p>"
        "<a href='/on'><button>BAT DEN</button></a> "
        "<a href='/off'><button>TAT DEN</button></a>"
        "</body></html>", status);

    // Ghi trực tiếp chuỗi HTTP vào tx_buf sau phần Header (14 + 20 + 20 = 54)
    int http_len = snprintf((char *)&tx_buf[54], sizeof(tx_buf) - 54,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n"
        "%s", body_len, html_body);

    // Tính toán độ dài động cho các tầng
    uint16_t tcp_len_field = 20 + http_len;       // Độ dài TCP Header + HTTP Data
    uint16_t ip_total_len = 20 + tcp_len_field;   // Độ dài IP Header + Toàn bộ TCP
    uint16_t total_frame_len = 14 + ip_total_len; // Tổng chiều dài đẩy ra dây mạng

    // =========================================================
    // 2. ETHERNET HEADER
    // =========================================================
    memcpy(&tx_buf[0], eth_handle->RX->Source_address, 6);

    tx_buf[6]  = (uint8_t)(my_mac >> 40);
    tx_buf[7]  = (uint8_t)(my_mac >> 32);
    tx_buf[8]  = (uint8_t)(my_mac >> 24);
    tx_buf[9]  = (uint8_t)(my_mac >> 16);
    tx_buf[10] = (uint8_t)(my_mac >> 8);
    tx_buf[11] = (uint8_t)(my_mac);

    tx_buf[12] = 0x08;
    tx_buf[13] = 0x00;

    // =========================================================
    // 3. IPv4 HEADER (Cập nhật độ dài động)
    // =========================================================
    tx_buf[14] = 0x45;
    tx_buf[15] = 0x00;

    // Ghi độ dài tổng IP động vừa tính
    tx_buf[16] = (uint8_t)(ip_total_len >> 8);
    tx_buf[17] = (uint8_t)(ip_total_len & 0xFF);

    tx_buf[18] = 0x00;
    tx_buf[19] = 0x00;
    tx_buf[20] = 0x40; // Don't Fragment
    tx_buf[21] = 0x00;
    tx_buf[22] = 0x80; // TTL = 128
    tx_buf[23] = 0x06; // Protocol = TCP
    tx_buf[24] = 0x00;
    tx_buf[25] = 0x00; // Checksum tạm thời bằng 0

    tx_buf[26] = (my_ip >> 24) & 0xFF;
    tx_buf[27] = (my_ip >> 16) & 0xFF;
    tx_buf[28] = (my_ip >> 8)  & 0xFF;
    tx_buf[29] = my_ip & 0xFF;

    memcpy(&tx_buf[30], &rx_ip[12], 4);

    // =========================================================
    // 4. TÍNH IP HEADER CHECKSUM
    // =========================================================
    uint32_t ip_sum = 0;
    for(int i = 14; i < 34; i += 2)
    {
        ip_sum += ((uint16_t)tx_buf[i] << 8) | tx_buf[i + 1];
    }
    while(ip_sum >> 16)
    {
        ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);
    }
    uint16_t ip_chk = (uint16_t)(~ip_sum);
    tx_buf[24] = ip_chk >> 8;
    tx_buf[25] = ip_chk & 0xFF;

    // =========================================================
    // 5. TCP HEADER
    // =========================================================
    tx_buf[34] = rx_tcp[2]; // Source Port (80)
    tx_buf[35] = rx_tcp[3];
    tx_buf[36] = rx_tcp[0]; // Dest Port PC
    tx_buf[37] = rx_tcp[1];

    tx_buf[38] = (tx_seq >> 24) & 0xFF;
    tx_buf[39] = (tx_seq >> 16) & 0xFF;
    tx_buf[40] = (tx_seq >> 8)  & 0xFF;
    tx_buf[41] = tx_seq & 0xFF;

    tx_buf[42] = (tx_ack >> 24) & 0xFF;
    tx_buf[43] = (tx_ack >> 16) & 0xFF;
    tx_buf[44] = (tx_ack >> 8)  & 0xFF;
    tx_buf[45] = tx_ack & 0xFF;

    tx_buf[46] = 0x50; // TCP Len = 20 bytes
    tx_buf[47] = 0x18; // THAY ĐỔI: Sử dụng cờ [PSH, ACK] để trình duyệt hiển thị data ngay lập tức

    tx_buf[48] = 0x08;
    tx_buf[49] = 0x00; // Window size
    tx_buf[50] = 0x00;
    tx_buf[51] = 0x00; // Checksum tạm thời bằng 0
    tx_buf[52] = 0x00;
    tx_buf[53] = 0x00; // Urgent pointer

    // =========================================================
    // 6. TÍNH TCP CHECKSUM KÈM PSEUDO HEADER VÀ HTTP DATA
    // =========================================================
    uint32_t tcp_sum = 0;

    // Pseudo Header: Source IP + Destination IP
    for(int i = 26; i < 34; i += 2)
    {
        tcp_sum += ((uint16_t)tx_buf[i] << 8) | tx_buf[i + 1];
    }
    tcp_sum += 0x0006;         // Protocol TCP
    tcp_sum += tcp_len_field;  // Độ dài TCP Header + HTTP gộp chung (Động)

    // Tính Checksum quét qua cả TCP Header lẫn toàn bộ vùng dữ liệu HTTP
    for(int i = 0; i < tcp_len_field; i += 2)
    {
        // Xử lý trường hợp tổng số byte TCP bị lẻ (Tránh đọc tràn bộ nhớ)
        if (i == tcp_len_field - 1)
        {
            tcp_sum += ((uint16_t)tx_buf[34 + i] << 8);
        }
        else
        {
            tcp_sum += ((uint16_t)tx_buf[34 + i] << 8) | tx_buf[34 + i + 1];
        }
    }

    while(tcp_sum >> 16)
    {
        tcp_sum = (tcp_sum & 0xFFFF) + (tcp_sum >> 16);
    }
    uint16_t tcp_chk = (uint16_t)(~tcp_sum);
    tx_buf[50] = tcp_chk >> 8;
    tx_buf[51] = tcp_chk & 0xFF;

    // =========================================================
    // 7. GỬI TOÀN BỘ FRAME LÊN DÂY MẠNG
    // =========================================================
    ETH_TX_TRANS(eth_handle, tx_buf, total_frame_len);

    eth_handle->TX = (ETH_TRANS_DATA *)tx_buf;
}*/
void ETH_PACKET_REPLY(ETH_Handle_t *eth_handle, uint64_t my_mac, uint32_t my_ip,PACKET_REPLY * packet_rep,uint16_t length)
{
	//------------------------handle packet--------------------------------------
	switch (packet_rep->packet_type)
	{
	case REPLY_TYPE_ARP:
	{
			ARP_Packet_t *rx_arp = (ARP_Packet_t *)eth_handle->RX;
			memcpy(packet_rep->arp_packet.eth.dest_mac, rx_arp->eth.src_mac, 6);
			   // memcpy(packet_rep->arp_packet.eth.src_mac, my_mac, 6);
			packet_rep->arp_packet.eth.src_mac[0]=(uint8_t)(my_mac>>40);
			packet_rep->arp_packet.eth.src_mac[1]=(uint8_t)(my_mac>>32);
			packet_rep->arp_packet.eth.src_mac[2]=(uint8_t)(my_mac>>24);
			packet_rep->arp_packet.eth.src_mac[3]=(uint8_t)(my_mac>>16);
			packet_rep->arp_packet.eth.src_mac[4]=(uint8_t)(my_mac>>8);
			packet_rep->arp_packet.eth.src_mac[5]=(uint8_t)(my_mac);
			packet_rep->arp_packet.eth.eth_type = __builtin_bswap16(0x0806); // ARP Type

			    // --- Phần ARP Payload ---
			    packet_rep->arp_packet.arp.htype = __builtin_bswap16(1);
			    packet_rep->arp_packet.arp.ptype = __builtin_bswap16(0x0800);
			    packet_rep->arp_packet.arp.hlen  = 6;
			    packet_rep->arp_packet.arp.plen  = 4;
			    packet_rep->arp_packet.arp.op    = __builtin_bswap16(2); // Reply

			    // Đảo cấu hình IP/MAC Người gửi - Người nhận
			    packet_rep->arp_packet.arp.sender_mac[0]=(uint8_t)(my_mac>>40);
			    packet_rep->arp_packet.arp.sender_mac[1]=(uint8_t)(my_mac>>32);
			    packet_rep->arp_packet.arp.sender_mac[2]=(uint8_t)(my_mac>>24);
			    packet_rep->arp_packet.arp.sender_mac[3]=(uint8_t)(my_mac>>16);
			    packet_rep->arp_packet.arp.sender_mac[4]=(uint8_t)(my_mac>>8);
			    packet_rep->arp_packet.arp.sender_mac[5]=(uint8_t)(my_mac);

			    packet_rep->arp_packet.arp.sender_ip = __builtin_bswap32(my_ip);

			    memcpy(packet_rep->arp_packet.arp.target_mac, rx_arp->arp.sender_mac, 6);
			    packet_rep->arp_packet.arp.target_ip = rx_arp->arp.sender_ip;
			    ETH_TX_TRANS(eth_handle,packet_rep->tx_buffer,length);
		break;
	}
	/*case REPLY_TYPE_TCP:
	    {
	        // 1. Ép kiểu dữ liệu vùng nhận (RX Data) thành cấu trúc IP và TCP để đọc
	        IP_Header_t  *rx_ip  = (IP_Header_t *)eth_handle->RX->data;

	        // Tính toán độ dài Header động để bóc tách chính xác (đề phòng gói nhận có Option)
	        uint8_t rx_ip_hlen   = (rx_ip->ver_ihl & 0x0F) * 4;
	        TCP_Header_t *rx_tcp = (TCP_Header_t *)((uint8_t *)rx_ip + rx_ip_hlen);
	        uint8_t rx_tcp_hlen  = ((rx_tcp->data_offset >> 4) & 0x0F) * 4;

	        // Trích xuất các thông số từ gói nhận để tính toán SEQ/ACK
	        uint16_t rx_ip_total_len = __builtin_bswap16(rx_ip->tot_len);
	        uint16_t tcp_payload_len = rx_ip_total_len - rx_ip_hlen - rx_tcp_hlen;
	        uint8_t  tcp_flags       = rx_tcp->flags;

	        uint32_t rx_seq          = __builtin_bswap32(rx_tcp->seq_num);
	        uint32_t rx_ack          = __builtin_bswap32(rx_tcp->ack_num);

	        // ---------------------------------------------------------
	        // 1. CẤU HÌNH LỚP ETHERNET CHUNG (14 Byte) - NẰM NGOÀI SWITCH
	        // ---------------------------------------------------------
	        memcpy(packet_rep->tcp_packet.eth.dest_mac, eth_handle->RX->Source_address, 6);

	        packet_rep->tcp_packet.eth.src_mac[0] = (uint8_t)(my_mac >> 40);
	        packet_rep->tcp_packet.eth.src_mac[1] = (uint8_t)(my_mac >> 32);
	        packet_rep->tcp_packet.eth.src_mac[2] = (uint8_t)(my_mac >> 24);
	        packet_rep->tcp_packet.eth.src_mac[3] = (uint8_t)(my_mac >> 16);
	        packet_rep->tcp_packet.eth.src_mac[4] = (uint8_t)(my_mac >> 8);
	        packet_rep->tcp_packet.eth.src_mac[5] = (uint8_t)(my_mac);

	        packet_rep->tcp_packet.eth.eth_type = __builtin_bswap16(ETH_TYPE_IPV4); // 0x0800

	        // ---------------------------------------------------------
	        // 2. CẤU HÌNH LỚP IP CHUNG (20 Byte) - ĐƯA RA NGOÀI ĐỂ DÙNG CHUNG
	        // ---------------------------------------------------------
	        // Vì cả gói SYN_ACK và ACK thuần đều dài tổng cộng 40 byte (20 IP + 20 TCP)
	        packet_rep->tcp_packet.ip.ver_ihl   = 0x45;
	        packet_rep->tcp_packet.ip.tos       = 0x00;
	        packet_rep->tcp_packet.ip.tot_len   = __builtin_bswap16(40);
	        packet_rep->tcp_packet.ip.id        = 0x0000;
	        packet_rep->tcp_packet.ip.frag_off  = __builtin_bswap16(0x4000); // Don't Fragment
	        packet_rep->tcp_packet.ip.ttl       = 128;
	        packet_rep->tcp_packet.ip.protocol  = IP_PROTO_TCP; // 0x06
	        packet_rep->tcp_packet.ip.checksum  = 0;
	        packet_rep->tcp_packet.ip.src_ip    = __builtin_bswap32(my_ip);
	        packet_rep->tcp_packet.ip.dest_ip   = rx_ip->src_ip;

	        // --- Tự tính IP Checksum chung luôn ---
	        uint32_t ip_sum = 0;
	        uint16_t *ip_ptr = (uint16_t *)&packet_rep->tcp_packet.ip;
	        for (int i = 0; i < 10; i++) {
	            ip_sum += __builtin_bswap16(ip_ptr[i]);
	        }
	        while (ip_sum >> 16) ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);
	        packet_rep->tcp_packet.ip.checksum = __builtin_bswap16((uint16_t)~ip_sum);

	        // ---------------------------------------------------------
	        // 3. CẤU HÌNH CÁC TRƯỜNG TCP CHUNG - ĐƯA RA NGOÀI ĐỂ DÙNG CHUNG
	        // ---------------------------------------------------------
	        packet_rep->tcp_packet.tcp.src_port    = rx_tcp->dest_port;
	        packet_rep->tcp_packet.tcp.dest_port   = rx_tcp->src_port;
	        packet_rep->tcp_packet.tcp.data_offset = 0x50; // 20 byte header
	        packet_rep->tcp_packet.tcp.window      = __builtin_bswap16(2048);
	        packet_rep->tcp_packet.tcp.checksum    = 0;
	        packet_rep->tcp_packet.tcp.urg_ptr     = 0;

	        // ---------------------------------------------------------
	        // 4. RẼ NHÁNH SWITCH-CASE: CHỈ ĐỔI CÁC TRƯỜNG KHÁC NHAU RIÊNG BIỆT
	        // ---------------------------------------------------------
	        switch(packet_rep->tcp_type)
	        {
	            case REPLY_TYPE_TCP_SYN_ACK:
	            {
	                // Sequence Number tự chọn ban đầu của STM32
	                packet_rep->tcp_packet.tcp.seq_num     = __builtin_bswap32(0x11223344);
	                // Gói SYN chiếm 1 đơn vị SEQ, nên ACK phát đi = SEQ nhận + 1
	                packet_rep->tcp_packet.tcp.ack_num     = __builtin_bswap32(rx_seq + 1);
	                packet_rep->tcp_packet.tcp.flags       = (TCP_FLAG_SYN | TCP_FLAG_ACK); // 0x12
	                break;
	            }

	            case REPLY_TYPE_TCP_ACK:
	            {
	                // Logic tính SEQ/ACK phản hồi gói tin chuẩn từ hàm cũ của bạn
	                uint32_t tx_seq = rx_ack;
	                uint32_t tx_ack = rx_seq + tcp_payload_len;

	                if (tcp_flags & 0x02) tx_ack++; // Nếu gói nhận chứa cờ SYN
	                if (tcp_flags & 0x01) tx_ack++; // Nếu gói nhận chứa cờ FIN

	                packet_rep->tcp_packet.tcp.seq_num     = __builtin_bswap32(tx_seq);
	                packet_rep->tcp_packet.tcp.ack_num     = __builtin_bswap32(tx_ack);
	                packet_rep->tcp_packet.tcp.flags       = TCP_FLAG_ACK; // 0x10 (Chỉ cờ ACK thuần)
	                break;
	            }
	            case REPLY_TYPE_FIN_ACK:
	            {
	            	// Thuật toán tính SEQ và ACK của gói FIN giống hệt gói ACK
	            	uint32_t tx_seq = rx_ack;
	            	uint32_t tx_ack = rx_seq + tcp_payload_len;

	            	// Nếu máy tính gửi FIN tới, ta phải cộng 1 vào ACK vì cờ FIN chiếm 1 đơn vị SEQ
	            	if (tcp_flags & 0x02) tx_ack++;
	            	if (tcp_flags & 0x01) tx_ack++;

	            	packet_rep->tcp_packet.tcp.seq_num     = __builtin_bswap32(tx_seq);
	            	packet_rep->tcp_packet.tcp.ack_num     = __builtin_bswap32(tx_ack);
	            	// BẬT CỜ FIN VÀ ACK (0x01 | 0x10 = 0x11)
	            	packet_rep->tcp_packet.tcp.flags       = (TCP_FLAG_FIN | TCP_FLAG_ACK);
	            	break;
	            }

	            default:
	                break;
	        }

	        // ---------------------------------------------------------
	        // 5. TỰ TÍNH TCP CHECKSUM KÈM PSEUDO-HEADER (DÙNG CHUNG)
	        // ---------------------------------------------------------
	        uint32_t tcp_sum = 0;

	        // Cộng Pseudo-Header: IP nguồn + IP đích
	        uint16_t *src_ip_ptr = (uint16_t *)&packet_rep->tcp_packet.ip.src_ip;
	        uint16_t *dst_ip_ptr = (uint16_t *)&packet_rep->tcp_packet.ip.dest_ip;
	        tcp_sum += __builtin_bswap16(src_ip_ptr[0]) + __builtin_bswap16(src_ip_ptr[1]);
	        tcp_sum += __builtin_bswap16(dst_ip_ptr[0]) + __builtin_bswap16(dst_ip_ptr[1]);

	        // Cộng Pseudo-Header: Protocol (0x0006) + TCP Length (20 byte = 0x0014)
	        tcp_sum += 0x0006;
	        tcp_sum += 0x0014;

	        // Cộng toàn bộ TCP Header thực tế (20 byte = 10 từ 16-bit)
	        uint16_t *tcp_hdr_ptr = (uint16_t *)&packet_rep->tcp_packet.tcp;
	        for (int i = 0; i < 10; i++) {
	            tcp_sum += __builtin_bswap16(tcp_hdr_ptr[i]);
	        }

	        // Chốt hạ Checksum
	        while (tcp_sum >> 16) tcp_sum = (tcp_sum & 0xFFFF) + (tcp_sum >> 16);
	        packet_rep->tcp_packet.tcp.checksum = __builtin_bswap16((uint16_t)~tcp_sum);

	        break;
	    }*/

	default:
		break;
	}
	//ETH_TX_TRANS(eth_handle,packet_rep->tx_buffer,length);
}
