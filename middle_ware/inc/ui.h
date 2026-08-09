/*
 * ui.h
 *
 *  Created on: Jul 20, 2026
 *      Author: ADMIN
 */

#ifndef INC_UI_H_
#define INC_UI_H_
#include <stdint.h>
#include "stm32f407.h"
#include"gpio.h"
#include"spi.h"
#include<string.h>
#include"uart.h"
#include"i2c.h"
#include"otg.h"
#include"timer.h"
#include"adc.h"
#include"eth.h"
#include "eth_hanlde_packet.h"
#include "dns.h"
#include "udp_layer.h" // Nhúng thêm nếu cần gọi các hàm UDP khác
#include "mqtt.h"
#include"tftcolor.h"
#include"dma.h"
#include "touch_handle.h"
#include "ff.h"
#include"sd.h"
#include "ui.h"
extern volatile uint8_t system_mode;
extern volatile uint8_t app_mode;
extern volatile uint8_t read_active;
extern SPI_Handle_t spi1;

// --- ĐỊNH NGHĨA CÁC CHỨC NĂNG CỦA MENU ---
void App_PlayVideo(void) {
    app_mode = 1;      // Chuyển sang trạng thái chiếu phim
    read_active = 1;   // Bật cờ cho luồng đọc FatFS
    tft_clear(&spi1, 0x0000); // Xóa đen màn hình
}

void App_EthernetMode(void) {
    system_mode = 0;   // Đẩy hệ thống sang Mode 0 (Ethernet)
    tft_clear(&spi1, 0x0000); // Xóa màn hình
    tft_write_string(&spi1, 80, 120, "ETHERNET MODE", 0xFFFF, 0x0000);
    // Khởi tạo phần cứng ETH ngay khi chuyển mode
    ETH_PIN_Init();
    ETH_INIT();
}

// --- CẤU TRÚC 1 Ô MENU ---
typedef void (*ActionFunc_t)(void);
typedef struct {
    uint16_t x, y;
    char text[15];
    ActionFunc_t on_select;
} MenuItem_t;

// Khai báo 2 nút chức năng to
#define MAX_ITEMS 2
MenuItem_t menu_items[MAX_ITEMS] = {
    { 50,  60,  "1. PLAY RAW", App_PlayVideo },
    { 50, 120,  "2. ETHERNET", App_EthernetMode }
};

static uint8_t current_item = 0;

// Hàm vẽ 1 dòng chữ (Highlight bằng cách đổi màu thành Đỏ)
void Draw_Single_Item(SPI_Handle_t* hspi, uint8_t index, uint8_t is_highlighted) {
    MenuItem_t *item = &menu_items[index];
    uint16_t text_color = is_highlighted ? 0xF800 : 0xFFFF; // Đỏ (đang chọn) hoặc Trắng

    // In dấu mũi tên > nếu đang chọn để đẹp hơn
    if (is_highlighted) tft_write_string(hspi, item->x - 20, item->y, ">", 0xF800, 0x0000);
    else tft_write_string(hspi, item->x - 20, item->y, " ", 0x0000, 0x0000); // Xóa mũi tên

    tft_write_string(hspi, item->x, item->y, item->text, text_color, 0x0000);
}

// Khởi tạo màn hình Menu ban đầu
void Menu_Init_Draw(SPI_Handle_t* hspi) {
    tft_clear(hspi, 0x0000);
    for(int i = 0; i < MAX_ITEMS; i++) {
        Draw_Single_Item(hspi, i, (i == current_item) ? 1 : 0);
    }
}

// Xử lý Sự kiện API từ Joystick
void Menu_ProcessEvent( MenuEvent_t event, SPI_Handle_t* hspi) {
    if (event == EVENT_NONE) return;

    uint8_t old_item = current_item;

    if (event == EVENT_DOWN) {
        if (current_item < MAX_ITEMS - 1) current_item++;
    }
    else if (event == EVENT_UP) {
        if (current_item > 0) current_item--;
    }
    else if (event == EVENT_RIGHT) {
        // Gạt sang PHẢI được tính là Lệnh SELECT
        if (menu_items[current_item].on_select != NULL) {
            menu_items[current_item].on_select();
        }
        return; // Thực thi xong thì thoát luôn, không vẽ lại UI nữa
    }

    // Nếu vị trí thay đổi, chỉ vẽ lại 2 ô (không chớp màn hình)
    if (current_item != old_item) {
        Draw_Single_Item(hspi, old_item, 0);    // Tắt sáng
        Draw_Single_Item(hspi, current_item, 1);// Bật sáng đỏ
    }
}

#endif /* INC_UI_H_ */
