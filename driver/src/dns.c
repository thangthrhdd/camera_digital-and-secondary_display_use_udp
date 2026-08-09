/*
 * dns.c
 *
 *  Created on: Jun 16, 2026
 *      Author: ADMIN
 */
#include "dns.h"
#include "udp_layer.h"
#include <string.h>

// Quản lý trạng thái nội bộ của tầng DNS
static volatile DNS_State_t dns_state = DNS_STATE_IDLE;
static uint32_t    dns_resolved_ip = 0;
static uint32_t    dns_server_ip = 0;
static uint8_t     gateway_mac[6] = {0};
static uint16_t    my_transaction_id = 0x1234; // ID tự chọn để khớp gói câu hỏi - câu trả lời

// Hàm khởi tạo thông tin DNS
void DNS_Init(uint32_t server_ip, uint8_t *gw_mac)
{
    dns_server_ip = server_ip;
    memcpy(gateway_mac, gw_mac, 6);
    dns_state = DNS_STATE_IDLE;
    dns_resolved_ip = 0;
}

// Hàm đóng gói và bắn câu hỏi DNS lên trời (gửi tới 8.8.8.8 chẳng hạn)
void DNS_Start_Query(ETH_Handle_t *eth_handle, const char *domain_name)
{
    uint8_t dns_req[512];
	//static uint8_t tx_buf[512] __attribute__((aligned(4)));
    memset(dns_req, 0, sizeof(dns_req));

    DNS_Header_t *header = (DNS_Header_t *)dns_req;
    header->id       = __builtin_bswap16(my_transaction_id);
    //header->flags    = DNS_FLAG_QUERY_STD; // Standard query (0x0100)
    // Sửa dòng gán flags thành:
    header->flags = __builtin_bswap16(DNS_FLAG_QUERY_STD);
    header->qd_count = __builtin_bswap16(1);   // Hỏi 1 tên miền

    // Mã hóa chuỗi "broker.hivemq.com" -> format DNS "\x06broker\x06hivemq\x03com\x00"
    int src_idx = 0;
    int dst_idx = 12; // Bắt đầu sau 12 byte Header
    int len_idx = dst_idx++;
    uint8_t char_count = 0;

    while (domain_name[src_idx] != '\0')
    {
        if (domain_name[src_idx] == '.')
        {
            dns_req[len_idx] = char_count;
            char_count = 0;
            len_idx = dst_idx++;
        }
        else
        {
            dns_req[dst_idx++] = domain_name[src_idx];
            char_count++;
        }
        src_idx++;
    }
    dns_req[len_idx] = char_count; // Điền độ dài từ cuối (com)
    dns_req[dst_idx++] = 0;        // Kết thúc chuỗi tên miền

    // Thêm Type A (Hỏi IPv4) và Class IN (Internet) vào đuôi
    dns_req[dst_idx++] = (DNS_TYPE_A >> 8) & 0xFF;   dns_req[dst_idx++] = DNS_TYPE_A & 0xFF;
    dns_req[dst_idx++] = (DNS_CLASS_IN >> 8) & 0xFF; dns_req[dst_idx++] = DNS_CLASS_IN & 0xFF;

    dns_state = DNS_STATE_QUERYING;

    // Bắn qua tầng UDP hạ tầng
    UDP_Send(eth_handle, gateway_mac, dns_server_ip, DNS_CLIENT_PORT, DNS_SERVER_PORT, dns_req, dst_idx);
}

// CHÍNH LÀ EM NÓ: Hàm xử lý gói tin DNS Server trả lời về cho STM32
// CHÍNH LÀ EM NÓ: Hàm xử lý gói tin DNS Server trả lời về cho STM32 (Bản cập nhật hỗ trợ nhiều Bản ghi / CNAME)
void DNS_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *udp_payload, uint16_t udp_len)
{
    if (udp_len < 12) return;

    DNS_Header_t *header = (DNS_Header_t *)udp_payload;

    // 1. Kiểm tra ID gói phản hồi có trùng khớp không
    if (__builtin_bswap16(header->id) != my_transaction_id) return;

    uint16_t flags = __builtin_bswap16(header->flags);
    uint16_t an_count = __builtin_bswap16(header->an_count);

    // Kiểm tra cờ lỗi (RCODE ở 4-bit thấp của flags), nếu lỗi hoặc không có Answer thì thoát
    if ((flags & 0x000F) != 0 || an_count == 0) {
        dns_state = DNS_STATE_ERROR;
        return;
    }

    uint32_t idx = 12; // Bỏ qua 12 byte Header ban đầu

    // 2. Bỏ qua chuỗi câu hỏi lặp lại trong phân đoạn Question Section
    while (udp_payload[idx] != 0) {
        idx++;
        if (idx >= udp_len) return;
    }
    idx += 5; // Nhảy qua nốt byte 0, Type (2B) và Class (2B) của Question Section

    // 3. VÒNG LẶP DUYỆT QUA TẤT CẢ CÁC BẢN GHI TRONG PHÂN ĐOẠN ANSWER
    for (int i = 0; i < an_count; i++)
    {
        if (idx >= udp_len) return;

        // THUẬT TOÁN ĐÃ SỬA: Nhảy qua vùng tên miền (Name) dạng lai hoặc nén an toàn
        while (1) {
            if (idx >= udp_len) return;

            if ((udp_payload[idx] & 0xC0) == 0xC0) {
                idx += 2; // Gặp con trỏ nén (0xC0xx), nhảy 2 byte rồi dừng quét Name
                break;
            } else if (udp_payload[idx] == 0x00) {
                idx++;    // Gặp ký tự kết thúc chuỗi thô, nhảy qua byte 0x00 rồi dừng quét
                break;
            } else {
                idx += (udp_payload[idx] + 1); // Nhảy qua độ dài phân đoạn chữ thô
            }
        }

        // Kiểm tra an toàn trước khi đọc dữ liệu cấu trúc cố định (Type, Class, TTL, RD_LEN)
        if (idx + 10 > udp_len) return;

        // Đọc loại bản ghi (Type)
        uint16_t answer_type = ((uint16_t)udp_payload[idx] << 8) | udp_payload[idx + 1];

        // Nhảy qua Type(2B) + Class(2B) + TTL(4B) = 8 byte để đến ô độ dài dữ liệu
        idx += 8;

        // Đọc độ dài dữ liệu RDATA (RDLENGTH)
        uint16_t rd_len = ((uint16_t)udp_payload[idx] << 8) | udp_payload[idx + 1];
        idx += 2; // idx dịch chuyển tới vị trí bắt đầu dữ liệu IP thực tế

        // Kiểm tra bảo vệ chống tràn bộ đệm dữ liệu gói tin
        if (idx + rd_len > udp_len) return;

        // 4. Nếu là bản ghi Loại A (IPv4) và độ dài dữ liệu chuẩn 4 byte, lấy IP đầu tiên tìm thấy
        if (answer_type == 0x0001 && rd_len == 4)
        {
            dns_resolved_ip = ((uint32_t)udp_payload[idx]     << 24) |
                              ((uint32_t)udp_payload[idx + 1] << 16) |
                              ((uint32_t)udp_payload[idx + 2] <<  8) |
                              ((uint32_t)udp_payload[idx + 3]);

            dns_state = DNS_STATE_SUCCESS; // Báo thành công về cho Tầng Ứng dụng MQTT!
            return;
        }
        else
        {
            // Nếu là CNAME hoặc loại bản ghi khác, bỏ qua để xét bản ghi tiếp theo
            idx += rd_len;
        }
    }

    // Nếu duyệt hết danh sách Answer mà không lấy được IPv4 hợp lệ
    dns_state = DNS_STATE_ERROR;
}
// Các hàm lấy thông tin trạng thái ra dùng ngoài main
DNS_State_t DNS_Get_State(void) { return dns_state; }
uint32_t DNS_Get_Resolved_IP(void) { return dns_resolved_ip; }
void DNS_Set_State(DNS_State_t new_state)
{
    dns_state = new_state;
}
