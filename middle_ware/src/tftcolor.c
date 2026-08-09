/*
 * tftcolor.c
 *
 *  Created on: Jul 4, 2026
 *      Author: ADMIN
 */
#include"tftcolor.h"
//#include "imge.h"

void cmd(SPI_Handle_t* spi, uint8_t data) {
    TFT_DC_CMD();       // 1. Kéo chân DC xuống 0 (Lệnh)
    TFT_CS_LOW();       // 2. Kéo chân CS xuống 0 (Chọn Chip)

    // 3. Truyền 1 byte qua SPI (thay thế bằng hàm SPI của bạn)
    SPI_Master_Transmit(spi->spix, &data, 1);

    TFT_CS_HIGH();      // 4. Kéo chân CS lên 1 (Hủy chọn)
}


void data_p(SPI_Handle_t* spi, uint8_t data) {
    TFT_DC_DATA();      // Kéo chân DC lên 1 (Dữ liệu)
    TFT_CS_LOW();
    SPI_Master_Transmit(spi->spix, &data, 1);
    TFT_CS_HIGH();
}


void tft_init(SPI_Handle_t* spi) {
    // --- Bước 1: Khởi tạo phần cứng (Hardware Reset) ---
    TFT_RST_HIGH();
    for(int i=0;i<10000;i++); // Đảm bảo chân Reset ở mức cao trước khi bắt đầu
    TFT_RST_LOW();
    for(int i=0;i<10000;i++);  // Datasheet yêu cầu TRW tối thiểu 10us [1]
    TFT_RST_HIGH();
    for(int i=0;i<120000;i++);  // Chờ tối thiểu 120ms (TRT) để chip tải xong cấu hình từ NVM [1]

    // --- Bước 2: Chuỗi lệnh cấu hình ST7789 ---

    // 1. Software Reset (0x01)
    cmd(spi, 0x01);
    for(int i=0;i<5000;i++); // Cần chờ ít nhất 5ms trước khi gửi lệnh tiếp theo [2]

    // 2. Sleep Out (0x11)
    cmd(spi, 0x11);
    for(int i=0;i<120000;i++);  // Bắt buộc chờ 120ms để mạch nguồn và dao động nội ổn định [3-5]

    // 3. Interface Pixel Format (0x3A)
    cmd(spi, 0x3A);
    data_p(spi, 0x55);  // Cấu hình 16-bit/pixel (RGB565) cho bộ nhớ khung [6-8]

    // 4. Memory Data Access Control (0x36)
    cmd(spi, 0x36);
    data_p(spi, 0x70);  // Xoay ngang màn hình (MX=1, MV=1, ML=1) [9, 10]

    // 5. Tần số quét tối đa (Frame Rate Control - 0xC6)
    // Nâng tần số quét lên mức cao nhất để hình ảnh mượt mà hơn
    cmd(spi, 0xC6);
    data_p(spi, 0x00);  // Giá trị 0x00 cấu hình tốc độ khung hình đạt 119Hz [11, 12]

    // 6. Bật tín hiệu Tearing Effect (TEON - 0x35)
    // Lệnh này kích hoạt tín hiệu đồng bộ trên chân TE của chip [9, 13, 14]
    cmd(spi, 0x35);
    data_p(spi, 0x00);  // Mode 1: Chỉ xuất thông tin V-Blanking [15]

    // 7. Display Inversion On (0x21)
    // Sửa lỗi: Bạn đang dùng 0x20 (Off), nên dùng 0x21 (On) cho màn hình IPS [16]
    cmd(spi, 0x20);

    // 8. Normal Display Mode On (0x13)
    cmd(spi, 0x13);     // Đưa màn hình về chế độ hiển thị bình thường [16-18]
    for(int i=0;i<10000;i++);

    // 9. Display On (0x29)
    cmd(spi, 0x29);     // Bật hiển thị chính thức [19]
    for(int i=0;i<120000;i++);       // Chờ hệ thống ổn định hoàn toàn
}
// Mỗi ký tự rộng 5 pixel, cao 8 pixel (1 byte mỗi cột)
const uint8_t Font5x7[][5] =
{
    // Uppercase
    ['A'] = {0x7E,0x11,0x11,0x11,0x7E},
    ['B'] = {0x7F,0x49,0x49,0x49,0x36},
    ['C'] = {0x3E,0x41,0x41,0x41,0x22},
    ['D'] = {0x7F,0x41,0x41,0x22,0x1C},
    ['E'] = {0x7F,0x49,0x49,0x49,0x41},
    ['F'] = {0x7F,0x09,0x09,0x09,0x01},
    ['G'] = {0x3E,0x41,0x49,0x49,0x7A},
    ['H'] = {0x7F,0x08,0x08,0x08,0x7F},
    ['I'] = {0x00,0x41,0x7F,0x41,0x00},
    ['J'] = {0x20,0x40,0x41,0x3F,0x01},
    ['K'] = {0x7F,0x08,0x14,0x22,0x41},
    ['L'] = {0x7F,0x40,0x40,0x40,0x40},
    ['M'] = {0x7F,0x02,0x0C,0x02,0x7F},
    ['N'] = {0x7F,0x04,0x08,0x10,0x7F},
    ['O'] = {0x3E,0x41,0x41,0x41,0x3E},
    ['P'] = {0x7F,0x09,0x09,0x09,0x06},
    ['Q'] = {0x3E,0x41,0x51,0x21,0x5E},
    ['R'] = {0x7F,0x09,0x19,0x29,0x46},
    ['S'] = {0x46,0x49,0x49,0x49,0x31},
    ['T'] = {0x01,0x01,0x7F,0x01,0x01},
    ['U'] = {0x3F,0x40,0x40,0x40,0x3F},
    ['V'] = {0x1F,0x20,0x40,0x20,0x1F},
    ['W'] = {0x3F,0x40,0x38,0x40,0x3F},
    ['X'] = {0x63,0x14,0x08,0x14,0x63},
    ['Y'] = {0x07,0x08,0x70,0x08,0x07},
    ['Z'] = {0x61,0x51,0x49,0x45,0x43},

    // Lowercase
    ['a'] = {0x20,0x54,0x54,0x54,0x78},
    ['b'] = {0x7F,0x48,0x44,0x44,0x38},
    ['c'] = {0x38,0x44,0x44,0x44,0x20},
    ['d'] = {0x38,0x44,0x44,0x48,0x7F},
    ['e'] = {0x38,0x54,0x54,0x54,0x18},
    ['f'] = {0x08,0x7E,0x09,0x01,0x02},
    ['g'] = {0x08,0x54,0x54,0x54,0x3C},
    ['h'] = {0x7F,0x08,0x04,0x04,0x78},
    ['i'] = {0x00,0x44,0x7D,0x40,0x00},
    ['j'] = {0x20,0x40,0x44,0x3D,0x00},
    ['k'] = {0x7F,0x10,0x28,0x44,0x00},
    ['l'] = {0x00,0x41,0x7F,0x40,0x00},
    ['m'] = {0x7C,0x04,0x18,0x04,0x78},
    ['n'] = {0x7C,0x08,0x04,0x04,0x78},
    ['o'] = {0x38,0x44,0x44,0x44,0x38},
    ['p'] = {0x7C,0x14,0x14,0x14,0x08},
    ['q'] = {0x08,0x14,0x14,0x18,0x7C},
    ['r'] = {0x7C,0x08,0x04,0x04,0x08},
    ['s'] = {0x48,0x54,0x54,0x54,0x20},
    ['t'] = {0x04,0x3F,0x44,0x40,0x20},
    ['u'] = {0x3C,0x40,0x40,0x20,0x7C},
    ['v'] = {0x1C,0x20,0x40,0x20,0x1C},
    ['w'] = {0x3C,0x40,0x30,0x40,0x3C},
    ['x'] = {0x44,0x28,0x10,0x28,0x44},
    ['y'] = {0x0C,0x50,0x50,0x50,0x3C},
    ['z'] = {0x44,0x64,0x54,0x4C,0x44},
	[' '] = {0x00,0x00,0x00,0x00,0x00},
	['!'] = {0x00,0x00,0x5F,0x00,0x00},
	['"'] = {0x00,0x07,0x00,0x07,0x00},
	['#'] = {0x14,0x7F,0x14,0x7F,0x14},
	['$'] = {0x24,0x2A,0x7F,0x2A,0x12},
	['%'] = {0x23,0x13,0x08,0x64,0x62},
	['&'] = {0x36,0x49,0x55,0x22,0x50},
	['\'']= {0x00,0x05,0x03,0x00,0x00},
	['('] = {0x00,0x1C,0x22,0x41,0x00},
	[')'] = {0x00,0x41,0x22,0x1C,0x00},
	['*'] = {0x14,0x08,0x3E,0x08,0x14},
	['+'] = {0x08,0x08,0x3E,0x08,0x08},
	[','] = {0x00,0x50,0x30,0x00,0x00},
	['-'] = {0x08,0x08,0x08,0x08,0x08},
	['.'] = {0x00,0x60,0x60,0x00,0x00},
	['/'] = {0x20,0x10,0x08,0x04,0x02},
	['?'] = {0x02,0x01,0x51,0x09,0x06},
	['@'] = {0x32,0x49,0x79,0x41,0x3E},
	['['] = {0x00,0x7F,0x41,0x41,0x00},
	['\\']= {0x02,0x04,0x08,0x10,0x20},
	[']'] = {0x00,0x41,0x41,0x7F,0x00},
	['^'] = {0x04,0x02,0x01,0x02,0x04},
	['_'] = {0x40,0x40,0x40,0x40,0x40},
	['`'] = {0x00,0x01,0x02,0x04,0x00},
	['{'] = {0x08,0x36,0x41,0x41,0x00},
	['|'] = {0x00,0x00,0x7F,0x00,0x00},
	['}'] = {0x00,0x41,0x41,0x36,0x08},
	['~'] = {0x08,0x04,0x08,0x10,0x08},

    // Numbers
    ['0'] = {0x3E,0x51,0x49,0x45,0x3E},
    ['1'] = {0x00,0x42,0x7F,0x40,0x00},
    ['2'] = {0x42,0x61,0x51,0x49,0x46},
    ['3'] = {0x21,0x41,0x45,0x4B,0x31},
    ['4'] = {0x18,0x14,0x12,0x7F,0x10},
    ['5'] = {0x27,0x45,0x45,0x45,0x39},
    ['6'] = {0x3C,0x4A,0x49,0x49,0x30},
    ['7'] = {0x01,0x71,0x09,0x05,0x03},
    ['8'] = {0x36,0x49,0x49,0x49,0x36},
    ['9'] = {0x06,0x49,0x49,0x29,0x1E},
};



// Hàm thiết lập vùng vẽ
void tft_set_window(SPI_Handle_t* spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    // 1. Cấu hình dải cột (Column Address Set - 0x2A)
    cmd(spi, 0x2A);
    data_p(spi, x1 >> 8);
    data_p(spi, x1 & 0xFF);
    data_p(spi, x2 >> 8);
    data_p(spi, x2 & 0xFF);

    // 2. Cấu hình dải hàng (Row Address Set - 0x2B)
    cmd(spi, 0x2B);
    data_p(spi, y1 >> 8);
    data_p(spi, y1 & 0xFF);
    data_p(spi, y2 >> 8);
    data_p(spi, y2 & 0xFF);

    // 3. Chuẩn bị ghi vào bộ nhớ RAM của màn hình (RAM Write - 0x2C)
    cmd(spi, 0x2C);
}
// Thử thay đổi kích thước lớn hơn
#define SCREEN_W 240
#define SCREEN_H 320 // Thử đổi 240 thành 320
void tft_clear(SPI_Handle_t* spi, uint16_t color) {
    // Chọn vùng vẽ toàn màn hình 240x240 (thay đổi nếu màn của bạn kích thước khác)
    tft_set_window(spi, 0, 0, SCREEN_H - 1, SCREEN_W - 1);

    uint8_t buffer[2];
    buffer[0] = color >> 8;   // Byte cao
    buffer[1] = color & 0xFF; // Byte thấp

    TFT_DC_DATA(); // Bật chế độ gửi dữ liệu xuyên suốt
    TFT_CS_LOW();  // Chọn chip

    // Đổ dữ liệu màu cho 240 * 240 pixel
    for (uint32_t i = 0; i < (SCREEN_W *  SCREEN_H); i++) {
    	SPI_Master_Transmit(spi->spix, buffer, 2); // Sửa từ spi thành spi->spix;
    }

    while((spi->spix->SR & (1 << 7)) != 0); // Chờ truyền xong byte cuối cùng
    TFT_CS_HIGH(); // Giải phóng chip
}
// Vẽ 1 ký tự lên màn hình tại tọa độ (x, y)
void tft_write_char(SPI_Handle_t* spi, uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color) {
    // Thiết lập vùng vẽ cho đúng kích thước của 1 ký tự (5x8 pixel)
    tft_set_window(spi, x, y, x + 5 - 1, y + 8 - 1);

    // Bật chân DC lên cao xuyên suốt vì chuẩn bị đổ hàng loạt dữ liệu màu sắc
    TFT_DC_DATA();
    TFT_CS_LOW(); // Chọn chip

    // Quét theo hàng dọc (8 pixel từ trên xuống)
    for (uint8_t row = 0; row < 8; row++) {
        // Quét theo hàng ngang (5 pixel từ trái sang)
        for (uint8_t col = 0; col < 5; col++) {
            // Kiểm tra xem bit tại vị trí này có được bật không
            uint16_t pixel_color = (Font5x7[(uint8_t)ch][col] & (1 << row)) ? color : bg_color;

            // Gửi 2 byte màu (RGB565) cho 1 pixel
            uint8_t buffer[2];
            buffer[0] = pixel_color >> 8;   // Byte cao
            buffer[1] = pixel_color & 0xFF; // Byte thấp
            SPI_Master_Transmit(spi->spix, buffer, 2);
        }
    }


    TFT_CS_HIGH();   // Giải phóng CS
}

// Hàm vẽ chuỗi ký tự (String)
void tft_write_string(SPI_Handle_t* spi, uint16_t x, uint16_t y, char* str, uint16_t color, uint16_t bg_color) {
    while (*str) {
        tft_write_char(spi, x, y, *str, color, bg_color);
        x += 6; // Dịch sang phải 6 pixel để viết chữ tiếp theo (5 pixel chữ + 1 pixel khoảng cách)
        str++;
    }
}

void tft_draw_image(SPI_Handle_t *hspi, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* image_data) {

    // 1. Thiết lập vùng vẽ tương ứng với kích thước ảnh
    tft_set_window(hspi, x, y, x + width - 1, y + height - 1); // Hàm window của bạn nhận vào SPI_Handle_t*

    // 2. Sử dụng macro để bật chân DC lên mức CAO (Gửi dữ liệu màu)
    TFT_DC_DATA();

    // 3. Sử dụng macro để kéo chân CS xuống THẤP (Chọn chip)
    TFT_CS_LOW();

    // 4. Tính tổng số byte dữ liệu cần truyền
    uint32_t total_bytes = (uint32_t)width * height * 2;

    // 5. SỬA TẠI ĐÂY: Phải truyền hspi->spix (SPI_RegDef_t*) vào hàm Transmit mới đúng chuẩn!
    SPI_Master_Transmit(hspi->spix, (uint8_t*)image_data, total_bytes);

    // 6. Chờ cờ BSY (Busy) hạ xuống 0 để dữ liệu kịp bay ra khỏi chân phần cứng hoàn toàn
    while ((hspi->spix->SR & (1 << 7)) != 0);

    // 7. Sử dụng macro để kéo chân CS lên CAO (Kết thúc phiên)
    TFT_CS_HIGH();
}
