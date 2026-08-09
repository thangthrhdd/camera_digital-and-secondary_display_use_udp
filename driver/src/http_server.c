/*
 * http_server.c
 *
 *  Created on: Jun 16, 2026
 *      Author: ADMIN
 */
#include "tcp_layer.h"
#include "eth.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>

void Process_HTTP_Request(ETH_Handle_t *eth_handle, TCP_Socket_t *sock)
{
    char tx_payload[512];
    char html_body[256];
    char *status = "HOME";

    char *rx_data = (char *)eth_handle->payload;

    // Logic điều khiển phần cứng dựa trên nội dung text nhận được
    if (strstr(rx_data, "GET /on") != NULL) {
        GPIO_WritePin(GPIOC, GPIO_PIN_6, SET);
        status = "ON";
    }
    else if (strstr(rx_data, "GET /off") != NULL) {
        GPIO_WritePin(GPIOC, GPIO_PIN_6, RESET);
        status = "OFF";
    }

    // Sinh chuỗi HTML
    int body_len = snprintf(html_body, sizeof(html_body),
        "<html><body><h1>STM32 Clean Web Server</h1><p>LED Status: <b>%s</b></p>"
        "<a href='/on'><button>ON</button></a> <a href='/off'><button>OFF</button></a></body></html>", status);

    int http_len = snprintf(tx_payload, sizeof(tx_payload),
        "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
        body_len, html_body);

    // RA LỆNH: Đưa cho tầng TCP gửi hộ bằng cờ PSH-ACK (0x18)
    TCP_Send(eth_handle, sock, (uint8_t*)tx_payload, http_len, 0x18);
}

