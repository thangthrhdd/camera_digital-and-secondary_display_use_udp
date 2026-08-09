/*
 * dns.h
 *
 *  Created on: Jun 16, 2026
 *      Author: ADMIN
 */

#ifndef INC_DNS_H_
#define INC_DNS_H_

#include <stdint.h>
#include "eth.h" // Nhúng file chứa cấu trúc ETH_Handle_t của bạn

// =========================================================================
// 1. CÁC ĐỊNH NGHĨA HẰNG SỐ (MACROS)
// =========================================================================
#define DNS_SERVER_PORT       53          // Cổng UDP chuẩn của DNS Server toàn cầu
#define DNS_CLIENT_PORT       55553       // Cổng UDP ngẫu nhiên của STM32 dùng để hỏi DNS

#define DNS_FLAG_QUERY_STD    0x0100      // Cờ gửi đi: Standard Query (Đã swap byte sẵn)
#define DNS_TYPE_A            0x0001      // Loại bản ghi: Hỏi IPv4 (Đã swap byte sẵn)
#define DNS_CLASS_IN          0x0001      // Lớp mạng: Internet (Đã swap byte sẵn)

// Các trạng thái của máy trạng thái DNS (Giúp code chạy non-blocking)
typedef enum {
    DNS_STATE_IDLE = 0,
    DNS_STATE_QUERYING,
    DNS_STATE_SUCCESS,
    DNS_STATE_ERROR
} DNS_State_t;

// =========================================================================
// 2. CẤU TRÚC DỮ LIỆU GÓI TIN (STRUCTS)
// =========================================================================

/* Cấu trúc cố định 12-byte Header của một gói tin DNS (bắt buộc packed) */
typedef struct{
    uint16_t id;          // Mã định danh (Transaction ID) để khớp câu hỏi với câu trả lời
    uint16_t flags;       // Các cờ cấu hình (Query/Response, Opcode, RCODE...)
    uint16_t qd_count;    // Số lượng câu hỏi gửi đi (Thường là 1)
    uint16_t an_count;    // Số lượng câu trả lời nhận về (Answer Resource Records)
    uint16_t ns_count;    // Số lượng bản ghi máy chủ quản lý danh nghĩa (Authority RRs)
    uint16_t ar_count;    // Số lượng bản ghi bổ sung (Additional RRs)
} __attribute__((packed))  DNS_Header_t;


// =========================================================================
// 3. KHAI BÁO CÁC HÀM GIAO TIẾP (API PROTOTYPES)
// =========================================================================

/**
 * @brief Khởi tạo thông tin DNS Server
 * @param server_ip: IP của DNS Server (Ví dụ: 0x08080808 cho 8.8.8.8)
 * @param gw_mac: MAC của Router/Gateway nhà bạn (vì gói tin phải đi qua Router để ra Internet)
 */
void DNS_Init(uint32_t server_ip, uint8_t *gw_mac);

/**
 * @brief Chủ động đóng gói và bắn câu hỏi DNS lên Server
 * @param eth_handle: Con trỏ quản lý Ethernet Driver của bạn
 * @param domain_name: Chuỗi tên miền cần hỏi (Ví dụ: "broker.hivemq.com")
 */
void DNS_Start_Query(ETH_Handle_t *eth_handle, const char *domain_name);

/**
 * @brief Hàm gác cổng nhận gói DNS. Hàm này sẽ được tầng UDP gọi sang khi nhận dữ liệu cổng 55553
 * @param eth_handle: Con trỏ quản lý Ethernet
 * @param udp_payload: Con trỏ trỏ trực tiếp vào vùng dữ liệu DNS (đã bóc vỏ UDP)
 * @param udp_len: Chiều dài dữ liệu DNS nhận được
 */
void DNS_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *udp_payload, uint16_t udp_len);

/**
 * @brief Hàm kiểm tra trạng thái và lấy IP đã phân giải thành công
 */
DNS_State_t DNS_Get_State(void);
uint32_t    DNS_Get_Resolved_IP(void);
void DNS_Set_State(DNS_State_t new_state) ;
#endif /* DNS_H_ */
