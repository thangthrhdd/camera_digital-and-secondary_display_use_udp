/*
 * mqtt.h
 *
 *  Created on: Jun 18, 2026
 *      Author: ADMIN
 */

#ifndef MQTT_H_
#define MQTT_H_
#include <stdint.h>
#include <string.h>
#include "eth.h"
#include "tcp_layer.h"

// =========================================================================
// CÁC ĐỊNH NGHĨA TRẠNG THÁI CỦA MQTT CLIENT
// =========================================================================
typedef enum {
    MQTT_STATE_DISCONNECTED = 0, // Chưa kết nối
    MQTT_STATE_DNS_RESOLVING,    // Đang chờ DNS phân giải IP
    MQTT_STATE_TCP_CONNECTING,   // Đang gửi gói TCP SYN
    MQTT_STATE_MQTT_CONNECTING,  // Đã thông TCP, đang gửi gói MQTT CONNECT
    MQTT_STATE_CONNECTED,         // Đã nhận CONNACK, sẵn sàng PUB/SUB
	MQTT_STATE_SUB,
	MQTT_STATE_PUB,
	MQTT_STATE_DATA,
	MQTT_STATE_IDLE
} MQTT_State_t;

// Cấu trúc quản lý phiên làm việc MQTT
typedef struct {
    MQTT_State_t    state;
    uint32_t        broker_ip;
    uint16_t        broker_port;
    char            client_id[32];
    char            username[32];   // <--- THÊM DÒNG NÀY ĐỂ CHỨA ACCESS TOKEN
    char payload[250];
    char id[16];
    TCP_Socket_Manager_t *tcp_sock;
} MQTT_Client_t;

// Biến toàn cục quản lý MQTT Client
extern MQTT_Client_t mqtt_client;

// =========================================================================
// KHAI BÁO CÁC HÀM API TẦNG ỨNG DỤNG
// =========================================================================

/**
 * @brief Khởi tạo kết nối MQTT (Gộp cả quá trình gọi DNS và bắt tay TCP)
 * @param url: Tên miền của Broker (Ví dụ: "broker.emqx.io") hoặc IP chuỗi
 * @param port: Cổng Broker (Thường là 1883)
 */
void MQTT_Init(ETH_Handle_t *eth_handle, const char *url, uint16_t port);

/**
 * @brief Gửi dữ liệu lên một Topic (QoS 0)
 * @param topic: Tên chủ đề
 * @param message: Nội dung tin nhắn
 */
void MQTT_Publish(ETH_Handle_t *eth_handle, const char *topic, const char *message);

/**
 * @brief Đăng ký theo dõi một Topic (QoS 0 hoặc 1)
 * @param topic: Tên chủ đề cần theo dõi
 */
void MQTT_Subscribe(ETH_Handle_t *eth_handle, const char *topic);

// =========================================================================
// HÀM PHỤ TRỢ XỬ LÝ NỘI BỘ (Được gọi từ tcp_layer.c)
// =========================================================================
void MQTT_Send_Connect_Packet(ETH_Handle_t *eth_handle);
void MQTT_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *mqtt_payload, uint16_t payload_len);
void MQTT_Handle(ETH_Handle_t *eth_handle,uint8_t state);
#endif /* MQTT_H_ */
