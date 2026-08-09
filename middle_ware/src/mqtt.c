/*
 * mqtt.c
 *
 *  Created on: Jun 18, 2026
 *      Author: ADMIN
 */
#include "dns.h"
#include "mqtt.h"
#include <stdlib.h>
#include "adc.h"
#include <stdio.h>
#include "gpio.h"
// Biến quản lý phiên MQTT (Tương tự như socket_80 của bạn)
MQTT_Client_t mqtt_client = { .state = MQTT_STATE_DISCONNECTED };
extern uint16_t bufff[1];
// =========================================================================
// 1. HÀM KHỞI TẠO VÀ BẮN GÓI TCP SYN
// =========================================================================
void MQTT_Init(ETH_Handle_t *eth_handle, const char *url, uint16_t port)
{
    // =========================================================
    // BƯỚC 1: TÌM KIẾM VÀ KHỞI TẠO SLOT TRỐNG
    // =========================================================
    TCP_Socket_Manager_t *slot = NULL;
    for(int i = 0; i < MAX_TCP_SOCKETS; i++) {
        if(!tcp_slots[i].is_active) {
            slot = &tcp_slots[i];
            break;
        }
    }

    // Nếu không còn slot nào trống, thoát hàm để tránh crash
    if (slot == NULL) return;

    // Xóa SẠCH SẼ toàn bộ dữ liệu cũ của slot này (Đây là cách memset đúng)
    memset(slot, 0, sizeof(TCP_Socket_Manager_t));

    // =========================================================
    // BƯỚC 2: GÁN THÔNG TIN QUẢN LÝ SOCKET
    // =========================================================
    slot->is_active = 1;

    slot->local_port = 18001+bufff[0]; // Cổng gửi đi của STM32 (Source Port)
    // Nếu trong TCP_Socket_Manager_t của bạn có biến remote_port, hãy gán luôn:
    // slot->remote_port = port;

    // =========================================================
    // BƯỚC 3: LIÊN KẾT MQTT CLIENT VÀO SLOT TCP
    // =========================================================
    // Gán con trỏ (Không dùng dấu & trước mqtt_client.tcp_sock)
    mqtt_client.tcp_sock = slot;
    mqtt_client.tcp_sock->header.state = TCP_SYN_SENT;
    mqtt_client.broker_port = port;
    strcpy(mqtt_client.client_id, "bde08b90-7109-11f1-8797-0936777895d2");
    strcpy(mqtt_client.username,"NKuo6gKr6f2NDJDGVmwL");

    // Giả sử IP đã phân giải được (35.172.255.228)
    //mqtt_client.broker_ip = 0x22F3D936;

    // =========================================================
    // BƯỚC 4: CẤU HÌNH TEMPLATE GÓI TIN MẠNG
    // =========================================================
    // --- MAC Nguồn ---
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[0] = (uint8_t)(eth_handle->eth_x.mac_addr >> 40);
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[1] = (uint8_t)(eth_handle->eth_x.mac_addr >> 32);
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[2] = (uint8_t)(eth_handle->eth_x.mac_addr >> 24);
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[3] = (uint8_t)(eth_handle->eth_x.mac_addr >> 16);
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[4] = (uint8_t)(eth_handle->eth_x.mac_addr >> 8);
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.source_mac[5] = (uint8_t)(eth_handle->eth_x.mac_addr);


    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[0] = 0x00;
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[1] = 0xE0;
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[2] = 0x4C;
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[3] =0x68;
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[4] = 0x01;
    mqtt_client.tcp_sock->header.TCP_HEAD.eth_head.des_mac[5] = 0xAA;
    // --- MAC Đích (RẤT QUAN TRỌNG) ---
    // Để gửi gói tin ra ngoài Internet (đến Broker), bạn PHẢI điền MAC của Modem/Router mạng nhà bạn vào đây.
    // Nếu để mảng 0, Router sẽ không nhận được gói tin đâu!
    // Ví dụ: memcpy(mqtt_client.tcp_sock->TCP_HEAD.eth_head.des_mac, gateway_mac, 6);

    // --- IP ---
    mqtt_client.tcp_sock->header.TCP_HEAD.ip_header.source_ip = __builtin_bswap32(eth_handle->eth_x.ip_addr);
    mqtt_client.tcp_sock->header.TCP_HEAD.ip_header.des_ip    = __builtin_bswap32(mqtt_client.broker_ip);

    // --- Port ---
    mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Source_Port = __builtin_bswap16(slot->local_port);
    mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Des_Port    = __builtin_bswap16(mqtt_client.broker_port);

    // --- Sequence / Ack ---
    mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(1000);
    mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Ack_Number = __builtin_bswap32(0);

    TCP_Send(&eth_handle, &mqtt_client.tcp_sock->header, NULL, 0, 0x04);
    for(volatile int i = 0; i < 500000; i++);
    // =========================================================
    // BƯỚC 5: CHUYỂN TRẠNG THÁI VÀ BẮN GÓI SYN
    // =========================================================
    mqtt_client.state = MQTT_STATE_TCP_CONNECTING;
    // Dùng TCP_SYN_RECEIVED hoặc TCP_SYN_SENT tùy máy trạng thái của bạn
    mqtt_client.tcp_sock->header.state = TCP_SYN_SENT;

    // Bắn gói TCP SYN (Cờ 0x02)
    TCP_Send(eth_handle, &mqtt_client.tcp_sock->header, NULL, 0, 0x02);

    // Tự động tăng Seq_Number lên 1 sau khi gửi SYN
   // uint32_t next_seq = __builtin_bswap32(mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Seq_Number) + 1;
   // mqtt_client.tcp_sock->header.TCP_HEAD.tcp_header.Seq_Number = __builtin_bswap32(next_seq);
}
// =========================================================================
// 2. HÀM GỬI GÓI MQTT CONNECT (Được gọi khi TCP báo Established)
// =========================================================================
void MQTT_Send_Connect_Packet(ETH_Handle_t *eth_handle)
{
    if (mqtt_client.state != MQTT_STATE_TCP_CONNECTING) return;

    uint8_t payload[160]; // Khai báo bộ đệm đủ rộng để chứa thêm Token
    uint16_t index = 0;

    // 1. Protocol Name Length (0x00 0x04) và Name ("MQTT")
    payload[index++] = 0x00; payload[index++] = 0x04;
    payload[index++] = 'M'; payload[index++] = 'Q'; payload[index++] = 'T'; payload[index++] = 'T';

    // 2. Protocol Level (v3.1.1 là 0x04)
    payload[index++] = 0x04;

    // 3. SỬA TẠI ĐY: Connect Flags phải là 0x82 (Username Flag + Clean Session)
    payload[index++] = 0x82;

    // 4. Keep Alive (60 giây -> 0x00 0x3C)
    payload[index++] = 0x00; payload[index++] = 0x3C;

    // 5. Đóng gói Client ID (Tên tự do, ví dụ "STM32")
    uint16_t client_len = strlen(mqtt_client.client_id);
    payload[index++] = (client_len >> 8) & 0xFF;
    payload[index++] = client_len & 0xFF;
    memcpy(&payload[index], mqtt_client.client_id, client_len);
    index += client_len;

    // 6. THÊM ĐOẠN ĐÓNG GÓI USERNAME (Chính là Access Token của ThingsBoard)
    uint16_t user_len = strlen(mqtt_client.username);
    payload[index++] = (user_len >> 8) & 0xFF; // Byte cao độ dài
    payload[index++] = user_len & 0xFF;        // Byte thấp độ dài
    memcpy(&payload[index], mqtt_client.username, user_len);
    index += user_len;

    // 7. Khởi tạo mảng gói CONNECT tổng chỉnh
    uint8_t connect_pkt[165];
    connect_pkt[0] = 0x10;  // Fixed Header: MQTT CONNECT Type
    connect_pkt[1] = index; // Remaining Length chuẩn (Tổng độ dài biến phía sau)
    memcpy(&connect_pkt[2], payload, index);

    mqtt_client.state = MQTT_STATE_MQTT_CONNECTING;

    // Bắn gói tin hoàn chỉnh qua tầng TCP (Độ dài tổng = index + 2 byte header)
    TCP_Send(eth_handle, &mqtt_client.tcp_sock->header, connect_pkt, index + 2, 0x18);
}

// =========================================================================
// 3. HÀM PUBLISH DỮ LIỆU
// =========================================================================
void MQTT_Publish(ETH_Handle_t *eth_handle, const char *topic, const char *message)
{
    if (mqtt_client.state <= MQTT_STATE_CONNECTED) return; // Chỉ gửi khi đã nhận CONNACK

    uint8_t payload[256];
    uint16_t topic_len = strlen(topic);
    uint16_t msg_len = strlen(message);
    uint16_t index = 0;

    // 1. Variable Header: Độ dài Topic + Topic
    payload[index++] = (topic_len >> 8) & 0xFF;
    payload[index++] = topic_len & 0xFF;
    memcpy(&payload[index], topic, topic_len);
    index += topic_len;

    // 2. Payload: Nội dung message
    memcpy(&payload[index], message, msg_len);
    index += msg_len;

    // 3. Fixed Header (PUBLISH, QoS 0)
    uint8_t total_packet[258];
    total_packet[0] = 0x30; // Type 3 = PUBLISH
    total_packet[1] = index; // Remaining Length

    memcpy(&total_packet[2], payload, index);

    // Bắn qua Tầng TCP
    TCP_Send(eth_handle, &mqtt_client.tcp_sock->header, total_packet, index + 2, 0x18);
    mqtt_client.state=MQTT_STATE_PUB;
}

// =========================================================================
// 4. HÀM SUBSCRIBE TOPIC
// =========================================================================
void MQTT_Subscribe(ETH_Handle_t *eth_handle, const char *topic)
{
    if (mqtt_client.state < MQTT_STATE_CONNECTED) return;

    uint8_t payload[128];
    uint16_t topic_len = strlen(topic);
    uint16_t index = 0;

    // 1. Packet Identifier (Bắt buộc cho QoS 1)
    payload[index++] = 0x00;
    payload[index++] = 0x01; // ID = 1

    // 2. Độ dài Topic + Topic
    payload[index++] = (topic_len >> 8) & 0xFF;
    payload[index++] = topic_len & 0xFF;
    memcpy(&payload[index], topic, topic_len);
    index += topic_len;

    // 3. Requested QoS
    payload[index++] = 0x00; // Đăng ký mức QoS 0

    // 4. Fixed Header (SUBSCRIBE)
    uint8_t total_packet[130];
    total_packet[0] = 0x82; // Type 8 = SUBSCRIBE (QoS 1)
    total_packet[1] = index;

    memcpy(&total_packet[2], payload, index);

    TCP_Send(eth_handle, &mqtt_client.tcp_sock->header, total_packet, index + 2, 0x18);

    mqtt_client.state=MQTT_STATE_SUB;
}

// =========================================================================
// 5. HÀM TIẾP NHẬN XỬ LÝ (Được gọi từ TCP_Process_Rx)
// =========================================================================
void MQTT_Process_Rx(ETH_Handle_t *eth_handle, uint8_t *mqtt_payload, uint16_t payload_len)
{
    // LOGIC MỚI: Nhận tín hiệu mồi từ tầng TCP (payload_len = 0) để bắn gói CONNECT
    if (payload_len == 0) {
        // TCP vừa thông xong, phải gửi gói MQTT CONNECT để chào Broker
        MQTT_Send_Connect_Packet(eth_handle);
        return;
    }

    // LOGIC CŨ: Phân tích các gói dữ liệu thực tế (payload_len >= 2)
    if (payload_len < 2) return;

    uint8_t packet_type = mqtt_payload[0] >> 4;

    switch (packet_type)
    {
        case 2: // Nhận được CONNACK (0x20)
            if (mqtt_payload[3] == 0x00) { // Return code = 0 (Connection Accepted)
                mqtt_client.state = MQTT_STATE_CONNECTED;
                // Có thể gọi hàm tự động Subscribe ngay tại đây:
                // MQTT_Subscribe(eth_handle, "stm32/cmd");
            }
            break;

        case 3: // Nhận được PUBLISH từ Broker
            {
            	uint16_t topic_len = (mqtt_payload[2] << 8) | mqtt_payload[3];
            	                uint8_t *message_ptr = &mqtt_payload[4 + topic_len];

            	                // 1. Trích xuất Topic an toàn ra mảng tạm
            	                char received_topic[64];
            	                if (topic_len > 63) topic_len = 63; // Chống tràn mảng tạm
            	                memcpy(received_topic, &mqtt_payload[4], topic_len);
            	                received_topic[topic_len] = '\0';   // Ép ký tự kết thúc chuỗi thô

            	                // 2. SỬA ĐOẠN LẤY ID: Tìm dấu '/' cuối cùng trên chuỗi Topic
            	                char *id_ptr = strrchr(received_topic, '/');
            	                if (id_ptr != NULL)
            	                {
            	                    id_ptr++; // Nhảy qua dấu '/' để đứng ngay tại vị trí số ID (Ví dụ: "15")

            	                    // Sao chép an toàn dữ liệu chuỗi ID vào struct mqtt_client
            	                    strncpy(mqtt_client.id, id_ptr, sizeof(mqtt_client.id) - 1);
            	                    mqtt_client.id[sizeof(mqtt_client.id) - 1] = '\0'; // Đảm bảo luôn có ngắt chuỗi
            	                }

            	                // 3. Tính độ dài và sao chép vùng Payload dữ liệu JSON
            	                uint16_t msg_len = mqtt_payload[1] - (2 + topic_len);
            	                if (msg_len > 0)
            	                {
            	                    // Giả sử mảng mqtt_client.payload có kích thước tối đa 128 byte
            	                    if (msg_len > 127) msg_len = 127; // Khống chế chống tràn bộ nhớ

            	                    memcpy(mqtt_client.payload, message_ptr, msg_len);
            	                    mqtt_client.payload[msg_len] = '\0'; // QUAN TRỌNG: Ép ký tự ngắt chuỗi cho Payload

            	                    // Đổi trạng thái báo cho hệ thống biết đã có data điều khiển mới
            	                    mqtt_client.state = MQTT_STATE_DATA;
            	                }

            }
            break;

        case 4: // Nhận được PUBACK
            break;

        case 9: // Nhận được SUBACK
            break;
    }
}
static void Handle_json(char * json)
{
	char * token=strtok(json,"{}:\"");
	char method[100];
	char params[100];
	while(token!=NULL)
	{
		if(strcmp(token,"method")==0)
		{
			token=strtok(NULL,"{}:\"");
			strcpy(method,token);
		}
		else if(strcmp(token,"params")==0)
		{
			token=strtok(NULL,"{}:\"");
			strcpy(params,token);
		}
		token=strtok(NULL,"{}:\"");
	}
	if(strcmp(method,"setState")==0)
	{
		if(strcmp(params,"true")==0)
		{
			GPIO_WritePin(GPIOC,GPIO_PIN_6,SET);
		}
		else if(strcmp(params,"false")==0)
		{
			GPIO_WritePin(GPIOC,GPIO_PIN_6,RESET);
		}
	}
	//printf("%s\n%s",method,params);

}
void MQTT_Handle(ETH_Handle_t * eth_handle,uint8_t state)
{
	switch (state)
	{
	case MQTT_STATE_CONNECTED:
        	MQTT_Publish(eth_handle,"v1/devices/me/telemetry","{\"temperature\":30}");
        	MQTT_Subscribe(eth_handle,"v1/devices/me/rpc/request/+");
		break;
	case MQTT_STATE_SUB:

		break;
	case MQTT_STATE_PUB:
		MQTT_Subscribe(eth_handle,"v1/devices/me/rpc/request/+");
		break;
	case MQTT_STATE_DATA:

		if(mqtt_client.id[0] != '\0')
		{

		char response_topic[64];
		            // Ghép chuỗi "v1/devices/me/rpc/response/" với số ID động vừa tìm được
		sprintf(response_topic, "v1/devices/me/rpc/response/%s",mqtt_client.id);
		MQTT_Publish(eth_handle,response_topic,"{\"status\":\"success\"}");
		Handle_json(mqtt_client.payload);
		mqtt_client.state = MQTT_STATE_IDLE;
		}
		break;
	default:
		break;
	}
}

