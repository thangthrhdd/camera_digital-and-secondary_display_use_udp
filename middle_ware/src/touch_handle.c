/*
 * touch_handle.c
 * Author: ADMIN
 */
#include "touch_handle.h"
#include "tftcolor.h"
#include "gpio.h"

// Sử dụng lại biến struct SPI bạn định nghĩa trong project
extern SPI_Handle_t spi1; // Nếu màn hình và touch dùng chung SPI2, hãy kiểm tra lại tên biến handle SPI ở main

const uint8_t red_dot_7x7[98] = {
    0x00,0x00, 0x00,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0x00,0x00,
    0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00,
    0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00,
    0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00,
    0x00,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0xF8,0x00, 0xF8,0x00, 0xF8,0x00, 0x00,0x00, 0x00,0x00
};

static volatile uint16_t y_poision=0, x_poision=0;
extern volatile uint8_t touch;

// Hàm đọc ADC bằng cách gọi ghi trực tiếp DR có giới hạn time-out chống treo máy
static uint16_t Touch_Read_Adc(uint8_t cmd) {
    uint8_t msb = 0, lsb = 0;
    volatile uint32_t timeout;

    GPIO_WritePin(GPIOD, GPIO_PIN_2, RESET); // Chọn chip Touch (CS = 0)

    // --- GỬI LỆNH VÀ ĐỢI CÓ GIỚI HẠN (TIMEOUT) ---
    timeout = 10000;
    while (!(SPI2->SR & (1 << 1))) { if (--timeout == 0) goto exit; } // Chờ TXE
    *(volatile uint8_t *)&SPI2->DR = cmd;

    timeout = 10000;
    while (!(SPI2->SR & (1 << 0))) { if (--timeout == 0) goto exit; } // Chờ RXNE
    (void)*(volatile uint8_t *)&SPI2->DR; // Đọc bỏ dữ liệu rác trả về khi gửi lệnh

    // Trễ nhỏ cho bộ ADC của IC XPT2046 chuyển đổi
    for(volatile int i = 0; i < 100; i++);

    // --- ĐỌC BYTE CAO (MSB) ---
    timeout = 10000;
    while (!(SPI2->SR & (1 << 1))) { if (--timeout == 0) goto exit; }
    *(volatile uint8_t *)&SPI2->DR = 0x00;

    timeout = 10000;
    while (!(SPI2->SR & (1 << 0))) { if (--timeout == 0) goto exit; }
    msb = *(volatile uint8_t *)&SPI2->DR;

    // --- ĐỌC BYTE THẤP (LSB) ---
    timeout = 10000;
    while (!(SPI2->SR & (1 << 1))) { if (--timeout == 0) goto exit; }
    *(volatile uint8_t *)&SPI2->DR = 0x00;

    timeout = 10000;
    while (!(SPI2->SR & (1 << 0))) { if (--timeout == 0) goto exit; }
    lsb = *(volatile uint8_t *)&SPI2->DR;

exit:
    GPIO_WritePin(GPIOD, GPIO_PIN_2, SET); // Nhả chip Touch (CS = 1)

    // Xử lý kết quả 12-bit
    uint16_t adc_val = (((uint16_t)msb << 8) | lsb) >> 3;
    return (adc_val & 0x0FFF);
}

void handle_touch(uint8_t touch_flag) {
    if (!touch_flag) return;

    // 1. Chuyển sang chế độ đọc an toàn (0x90 đọc X, 0xD0 đọc Y thông dụng)
    uint16_t raw_x = Touch_Read_Adc(0x90);
    uint16_t raw_y = Touch_Read_Adc(0xD0);

    // 2. Kiểm tra xem giá trị đọc về có hợp lệ không (Nếu bằng 0 hoặc 4095 tức là không chạm hoặc lỗi dây)
    if (raw_x > 150 && raw_x < 3950 && raw_y > 150 && raw_y < 3950) {

        // Cấu hình Calib cơ bản để test
        #define TOUCH_X_MIN   200
        #define TOUCH_X_MAX   3850
        #define TOUCH_Y_MIN   200
        #define TOUCH_Y_MAX   3800

        if (raw_x < TOUCH_X_MIN) raw_x = TOUCH_X_MIN;
        if (raw_x > TOUCH_X_MAX) raw_x = TOUCH_X_MAX;
        if (raw_y < TOUCH_Y_MIN) raw_y = TOUCH_Y_MIN;
        if (raw_y > TOUCH_Y_MAX) raw_y = TOUCH_Y_MAX;

        // Nội suy sang ma trận điểm 320x240
        uint16_t x_mapped = (uint32_t)(raw_x - TOUCH_X_MIN) * 319 / (TOUCH_X_MAX - TOUCH_X_MIN);
        uint16_t y_mapped = (uint32_t)(raw_y - TOUCH_Y_MIN) * 239 / (TOUCH_Y_MAX - TOUCH_Y_MIN);

        x_poision = x_mapped;
        y_poision = y_mapped;

        int16_t draw_x = x_poision - 3;
        int16_t draw_y = y_poision - 3;
        if (draw_x < 0) draw_x = 0;
        if (draw_y < 0) draw_y = 0;
        if (draw_x > 313) draw_x = 313;
        if (draw_y > 233) draw_y = 233;

        // Vẽ thử lên màn hình xem có chấm đỏ xuất hiện không
        tft_draw_image(&spi1, draw_x, draw_y, 7, 7, red_dot_7x7);
    }
}
