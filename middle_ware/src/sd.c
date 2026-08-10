/*
 * sd.c
 *
 * Created on: Jul 16, 2026
 * Author: ADMIN
 */
#include"sd.h"
#include "ff.h"           // Thư viện FatFS
#include "udp_layer.h"    // Để gọi hàm Push và IsFull
// Các định nghĩa mã lệnh (command index)
#define CMD0   0
#define CMD8   8
#define CMD17  17
#define CMD24  24
#define CMD55  55
#define ACMD41 41

SD_PACKET sd; // Định nghĩa biến tại đây

/*
 * HÀM HỖ TRỢ: Truyền nhận 1 byte đồng thời qua SPI1
 * Giúp tạo xung clock chuẩn xác và tự động xóa cờ nhận RXNE rác
 */
uint8_t SPI_Transfer(uint8_t data,SPI_RegDef_t * spiport) {
    uint32_t timeout;

    // 1. Kiểm tra và xóa lỗi Overrun
    if (spiport->SR & (1 << 6)) {
        volatile uint8_t dummy = spiport->DR;
        dummy = spiport->SR;
        (void)dummy;
    }

    // --- BẤT TỬ HÓA SPI: ÉP LUÔN Ở MASTER VÀ LUÔN BẬT ---
    spiport->CR1 |= (1 << 9) | (1 << 8); // Ép SSM=1, SSI=1 (Chống Mode Fault)
    spiport->CR1 |= (1 << 2);            // Ép MSTR=1 (Master Mode)
    spiport->CR1 |= (1 << 6);            // Ép SPE=1 (Bật SPI1)

    // 2. Đợi TXE (Transmit Empty) với Timeout
    timeout = 200000;
    while (!( spiport->SR & (1 << 1))) {
        if (--timeout == 0) return 0xFF; // Tránh treo MCU nếu clock SPI chết
    }

    // 3. Ghi dữ liệu
    spiport->DR = data;

    // 4. Đợi RXNE (Receive Not Empty) với Timeout
    timeout = 200000;
    while (!( spiport->SR & (1 << 0))) {
        if (--timeout == 0) return 0xFF; // Tránh treo MCU nếu không có phản hồi
    }

    return (uint8_t)( spiport->DR & 0xFF);
}

/*
 * Hàm SD_CMD chuẩn: Chỉ dùng cho các lệnh cấu hình ngắn (CMD0, CMD8, CMD55, ACMD41)
 */
uint8_t SD_CMD(uint8_t cmd, uint32_t arg)
{
    sd.cmd.cmd = 0x40 + cmd;
    sd.cmd.arg = __builtin_bswap32(arg);

    if (cmd == 0) {
        sd.cmd.crc = 0x95; // CMD0
    } else if (cmd == 8) {
        sd.cmd.crc = 0x87; // CMD8
    } else {
        sd.cmd.crc = 0x01; // Các lệnh khác
    }

    //GPIO_WritePin(GPIOD,GPIO_PIN_2,RESET);
    CS_RESET();

    // Gửi gói lệnh 6 bytes ra ngoài bằng SPI_Transfer
    for (int i = 0; i < 6; i++) {
        SPI_Transfer(sd.buffer[i],SPI_PORT);
    }

    // Đợi phản hồi R1 từ thẻ (quét tối đa 64 lần)
    uint8_t response = 0xFF;
    uint8_t retry = 0;
    while(retry < 64) {
        response = SPI_Transfer(0xFF,SPI_PORT);
        if(response != 0xFF) {
            break;
        }
        retry++;
    }

    //GPIO_WritePin(GPIOD,GPIO_PIN_2,SET);
    CS_SET();

    // Gửi thêm 8 xung nhịp phụ giải phóng bus
    SPI_Transfer(0xFF,SPI_PORT);

    return response;
}

uint8_t SD_Init(void) {
    // 1. Gửi tối thiểu 74 xung Clock để khởi động
	//GPIO_WritePin(GPIOD,GPIO_PIN_2,SET);
	CS_SET();
    for(int i = 0; i < 10; i++) {
        SPI_Transfer(0xFF,SPI_PORT);
    }

    for(volatile int delay = 0; delay < 1000; delay++);

    // 2. CMD0: Reset thẻ vào chế độ SPI
    uint8_t response = 0xFF;
    uint32_t retry = 0;
    while (retry < 100) {
        response = SD_CMD(CMD0, 0);
        if (response == 0x01) {
            break;
        }
        retry++;
    }

    if (response != 0x01) {
        return 1;
    }

    // 3. CMD8: Kiểm tra điện áp
    SD_CMD(CMD8, 0x000001AA);

    // 4. Vòng lặp khởi tạo (ACMD41)
    retry = 0;
    while (retry < 1000) {
        SD_CMD(CMD55, 0);
        if (SD_CMD(ACMD41, 0x40000000) == 0x00) {
            break;
        }
        retry++;
    }

    if (retry >= 1000) {
        return 2;
    }

    return 0;
}

/*
 * HÀM GHI SECTOR CHUẨN: Giữ CS LOW trong suốt quá trình ghi
 */
uint8_t SD_Write_Sector(uint32_t sector_address, uint8_t *data_buffer) {
    uint8_t response = 0xFF;
    uint8_t retry = 0;

    // Chuẩn bị gói lệnh CMD24
    sd.cmd.cmd = 0x40 + CMD24;
    sd.cmd.arg = __builtin_bswap32(sector_address);
    sd.cmd.crc = 0x01;

    //GPIO_WritePin(GPIOD,GPIO_PIN_2,RESET);// BẮT ĐẦU PHIÊN (CS xuống THẤP)
    CS_RESET();
    // 1. Gửi gói lệnh CMD24
    for (int i = 0; i < 6; i++) {
        SPI_Transfer(sd.buffer[i],SPI_PORT);
    }

    // 2. Đợi phản hồi R1
    while(retry < 64) {
        response = SPI_Transfer(0xFF,SPI_PORT);
        if(response != 0xFF) {
            break;
        }
        retry++;
    }

    // Nếu lệnh bị từ chối
    if (response != 0x00) {
    	//GPIO_WritePin(GPIOD,GPIO_PIN_2,SET);
    	CS_SET();
        SPI_Transfer(0xFF,SPI_PORT);
        return response;
    }

    // Gửi 1 byte dummy đồng bộ nhịp clock trước khi truyền Data Token
    SPI_Transfer(0xFF,SPI_PORT);

    // Gửi Data Token bắt đầu ghi (0xFE)
    SPI_Transfer(0xFE,SPI_PORT);

    // Gửi đủ 512 bytes dữ liệu
    for (int i = 0; i < 512; i++) {
        SPI_Transfer(data_buffer[i],SPI_PORT);
    }

    // Gửi 2 bytes CRC giả
    SPI_Transfer(0xFF,SPI_PORT);
    SPI_Transfer(0xFF,SPI_PORT);

    // Nhận byte phản hồi trạng thái ghi (Data Response)
    uint8_t data_resp = SPI_Transfer(0xFF,SPI_PORT);

    // Đợi thẻ ghi xong hoàn toàn (Busy Check)
    uint8_t busy;
    uint32_t timeout = 0;
    do {
        busy = SPI_Transfer(0xFF,SPI_PORT);
        timeout++;
        if (timeout > 500000) break;
    } while (busy != 0xFF);

    //GPIO_WritePin(GPIOD,GPIO_PIN_2,SET); // KẾT THÚC PHIÊN GHI (CS lên CAO)
    CS_SET();
    SPI_Transfer(0xFF,SPI_PORT); // Clock phụ giải phóng bus

    // Kiểm tra phản hồi ghi (0x05 là thành công)
    if ((data_resp & 0x1F) == 0x05) {
        return 0; // Trả về 0 (Thành công)
    } else {
        return 0xFF;
    }
}

/*
 * HÀM ĐỌC SECTOR CHUẨN: Giữ CS LOW liên tục từ lúc gửi lệnh CMD17 đến khi đọc xong CRC
 */
static uint8_t response=0;
uint8_t SD_Read_Sector(uint32_t sector_address, uint8_t *data_buffer) {
    //uint8_t response;
    volatile uint8_t temp;

    // Hút sạch rác
    while (SPI_PORT->SR & (1 << 0)) { temp = SPI_PORT->DR; }
    (void)temp;

    // Chuẩn bị gói lệnh CMD17
    sd.cmd.cmd = 0x40 + CMD17;
    sd.cmd.arg = __builtin_bswap32(sector_address);
    sd.cmd.crc = 0x01;

    // --- BƯỚC 1: RESET STATE MACHINE CỦA THẺ SD (SLEDGEHAMMER FLUSH) ---
    // Ép CS lên CAO và nhồi 80 xung clock (10 byte 0xFF).
    // Theo chuẩn SD, điều này ép thẻ hủy mọi trạng thái lơ lửng và quay về chờ lệnh mới.
    //GPIO_WritePin(GPIOD, GPIO_PIN_2, SET);
    CS_SET();
    for(int i = 0; i < 10; i++) {
        SPI_Transfer(0xFF,SPI_PORT);
    }

    // --- BƯỚC 2: BẮT ĐẦU PHIÊN ---
    //GPIO_WritePin(GPIOD, GPIO_PIN_2, RESET);
    CS_RESET();
    SPI_Transfer(0xFF,SPI_PORT); // Byte mồi căn lề trước khi gửi lệnh

    // Gửi 6 bytes lệnh CMD17
    for (int i = 0; i < 6; i++) {
        SPI_Transfer(sd.buffer[i],SPI_PORT);
    }

    // --- BƯỚC 3: TĂNG TIMEOUT LÊN MỨC CAO NHẤT VÀ DÙNG uint32_t ---
    uint32_t retry = 0;
    while(retry < 500000) {
        response = SPI_Transfer(0xFF,SPI_PORT);
        if(response != 0xFF) break;
        retry++;
    }

    // --- BƯỚC 4: BẪY LỖI ĐỂ TRUY TÌM NGUYÊN NHÂN ---
    if(response != 0x00) {
        //GPIO_WritePin(GPIOD, GPIO_PIN_2, SET);
    	CS_SET();
        SPI_Transfer(0xFF,SPI_PORT);

        // 🔥 ĐẶT BREAKPOINT Ở ĐÂY NẾU BỊ TREO!
        // Hãy xem giá trị của biến 'response' là bao nhiêu:
        // - Nếu là 0xFF: Thẻ SD chết lâm sàng (Timeout hoàn toàn).
        // - Nếu là 0x04: Nhiễu bus làm sai lệch lệnh (Illegal Command).
        // - Nếu là 0x08: Lỗi CRC do nhiễu tín hiệu MOSI.
        // - Nếu là 0x01: Thẻ bị rớt về trạng thái IDLE.
        return response;
    }
    // 3. Chờ Data Token bắt đầu (0xFE)
    uint32_t timeout = 0;
    uint8_t token = 0xFF;
    while(token != 0xFE) {
        token = SPI_Transfer(0xFF,SPI_PORT);
        timeout++;
        if(timeout > 800000) { // Tăng timeout chờ dữ liệu từ thẻ
            //GPIO_WritePin(GPIOD, GPIO_PIN_2, SET);
        	CS_SET();
            SPI_Transfer(0xFF,SPI_PORT);
            return 2; // Lỗi timeout không tìm thấy Data Token
        }
    }

    // 4. Nhận đủ 512 bytes dữ liệu từ thẻ
    for (int i = 0; i < 512; i++) {
        data_buffer[i] = SPI_Transfer(0xFF,SPI_PORT);
    }

    // 5. Đọc bỏ 2 bytes CRC rác
    SPI_Transfer(0xFF,SPI_PORT);
    SPI_Transfer(0xFF,SPI_PORT);

    //GPIO_WritePin(GPIOD, GPIO_PIN_2, SET); // HOÀN THÀNH (CS lên CAO)
    CS_SET();

    // Gửi thêm 8 xung nhịp phụ để giải phóng đường truyền hoàn toàn
    SPI_Transfer(0xFF,SPI_PORT);

    return 0; // Thành công
}
// Biến quản lý file tĩnh để giữ trạng thái file qua các lần gọi hàm
static FIL img_file;
static uint8_t is_file_open = 0;
static uint16_t current_pair_id = 0;

/**
 * @brief Đọc ảnh không chặn (Non-blocking). Mỗi lần gọi chỉ nạp 1 dải dòng vào Ring Buffer nếu còn chỗ.
 * @param filename: Tên file ảnh (chỉ dùng khi bắt đầu mở file)
 * @return 0: Đang xử lý, 1: Đã hoàn thành đọc toàn bộ ảnh, -1: Lỗi mở file
 */
int LoadImage_SD_To_TFT_NonBlocking(const char* filename) {
    FRESULT res;
    UINT bytesRead;
    static uint8_t temp_sd_buffer[1280] __attribute__((aligned(4)));

    // 1. Nếu file chưa mở, tiến hành mở file và reset bộ đếm
    if (!is_file_open) {
        res = f_open(&img_file, filename, FA_READ);
        if (res != FR_OK) {
            return -1; // Lỗi không mở được file
        }
        is_file_open = 1;
        current_pair_id = 0;
    }

    // 2. Nếu Ring Buffer đã đầy, thoát ra ngay lập tức để luồng main đi vẽ
    if (TFT_RingBuffer_IsFull()) {
        return 0; // Thư thả quay lại sau
    }

    // 3. Nếu Ring Buffer còn chỗ, đọc đúng 1 dải dòng (1280 bytes)
    // LƯU Ý: Thư viện FatFS tự động quản lý chân CS của thẻ SD bên trong hàm f_read!
    res = f_read(&img_file, temp_sd_buffer, 1280, &bytesRead);

    if (res == FR_OK && bytesRead == 1280) {
        // Đẩy vào Ring Buffer
        TFT_RingBuffer_Push(current_pair_id, temp_sd_buffer);
        current_pair_id++;
    }

    // 4. Kiểm tra điều kiện kết thúc
    if (res != FR_OK || bytesRead < 1280) {
        f_close(&img_file);
        is_file_open = 0;
        return 1; // Hoàn thành toàn bộ ảnh hoặc lỗi đọc kết thúc file
    }

    return 0;
}
#define MAX_FILES 10
char file_list[MAX_FILES][15];
int total_files = 0;

void Render_Menu(SPI_Handle_t* hspi) {
    DIR dir;
    FILINFO fno;
    total_files = 0;

    // 1. Xóa màn hình bằng hàm có sẵn
    tft_clear(hspi, 0x0000); // Màu đen

    // 2. Liệt kê file
    if (f_opendir(&dir, "/") == FR_OK) {
        int y_pos = 20;
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            if (!(fno.fattrib & AM_DIR) && strstr(fno.fname, ".RAW")) {

                // Vẽ tên file ra màn hình
                tft_write_string(hspi, 10, y_pos, fno.fname, 0xFFFF, 0x0000);

                y_pos += 20; // Cách nhau 20 pixel
                total_files++;
                if (total_files >= MAX_FILES) break;
            }
        }
        f_closedir(&dir);
    }
}
// --- CÁC BIẾN QUẢN LÝ TRẠNG THÁI ---
volatile uint8_t system_mode = 1; // 1 = Giao diện Menu, 0 = Ethernet
volatile uint8_t app_mode = 0;    // 0 = Đang ở màn hình Menu, 1 = Đang chạy Video (RAW)



// Hàm đọc ADC và sinh sự kiện 1 lần (Edge detection)
MenuEvent_t adc_handle(uint16_t bufff[4])
{
    uint16_t x_p = 0, y_p = 0;

    // Xóa cờ ngắt ADC nếu có
    if (ADC1->SR & (1 << 5)) {
        ADC1->SR &= ~(1 << 5);
        ADC1->CR2 |= (1 << 30);
    }

    x_p = bufff[1];
    y_p = bufff[2];

    MenuEvent_t current_state = EVENT_NONE;
    static uint8_t is_released = 1;

    // 1. Kiểm tra nút nhấn SW (SELECT)
    if (bufff[4] == EVENT_SELECT) {
        return EVENT_SELECT;
    }//sửa ở đây cho cam

    // 2. Mở rộng vùng trung tâm (Deadzone từ 1500 đến 3500)
    // Giúp triệt tiêu hoàn toàn độ rung/quán tính lò xo khi cần gạt bật về giữa
    if (x_p >= 3000) {
        current_state = EVENT_UP;
    }
    else if (x_p <= 600) {
        current_state = EVENT_DOWN;
    }
    else if (y_p >= 3000) {
        current_state = EVENT_LEFT;
    }
    else if (y_p <= 600) {
        current_state = EVENT_RIGHT;
    }
    else if (x_p > 1500 && x_p < 2500 && y_p > 1500 && y_p < 2500) {
        // Chỉ khi cần gạt thực sự nằm sâu bên trong vùng an toàn này mới được tính là ĐÃ THẢ
        is_released = 1;
        return EVENT_NONE;
    }
    else {
        // Vùng đệm trung gian, bỏ qua để tránh nhận nhầm
        return EVENT_NONE;
    }

    // 3. Chỉ trả về sự kiện khi cần đã được thả hẳn trước đó
    if (current_state != EVENT_NONE && is_released == 1) {
        is_released = 0; // Khóa lại ngay lập tức
        return current_state;
    }

    return EVENT_NONE;
}
