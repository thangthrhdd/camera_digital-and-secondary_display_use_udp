#include "menu.h"
#include "tftcolor.h"
#include <string.h>
#include <stdlib.h>
#include "ff.h"

MenuNode *root = NULL;
MenuNode *current_node = NULL;
volatile App_state_t current_app=Menu_Event;
extern ETH_Handle_t eth;
extern void ETH_PIN_Init(void);
extern void ETH_INIT(void);

// Quản lý trạng thái hệ thống
volatile SystemMode_t current_sys_mode = MODE_SD;
volatile SDAppMode_t current_sd_app = SD_APP_MENU;
extern volatile uint8_t read_active;

// Lấy biến FatFS và trạng thái đã Mount từ main (tfteth_2.c)
extern FATFS fs;
extern volatile uint8_t sd_mounted;

// =============================================================
// BỘ NHỚ LƯU TÊN FILE VÀ CỜ KHÓA QUÉT SD
// =============================================================
#define MAX_SD_FILES 15
static char sd_filenames[MAX_SD_FILES][20]; // Lưu tĩnh tên file vào RAM
static uint8_t sd_file_count = 0;
static uint8_t sd_scanned_success = 0;      // CỜ KHÓA: 0 = Chưa quét, 1 = Đã quét xong vĩnh viễn

extern DCMI_Handle_t dcmi;
// -------------------------------------------------------------
// 1. HÀM ACTION
// -------------------------------------------------------------
void App_PlayVideo_Action(void)
{
    current_sys_mode = MODE_SD;
    current_sd_app = SD_APP_PLAY_RAW;
    read_active = 1; // Bật cờ sẵn sàng đọc file từ SD
    current_app=SD_IMGE_Handle;
}

void App_Ethernet_Action(void)
{
    current_sys_mode = MODE_ETH;
    ETH_PIN_Init();
    ETH_INIT();

    tft_clear(&spi1, 0x0000);
    tft_write_string(&spi1, 80, 120, "ETHERNET MODE", 0xFFFF, 0x0000);
    current_app= ETH_Handle;
}
static uint8_t start_cam_flag=0;
void App_Camera_Actiton(void)
{
	current_sys_mode = MODE_CAM;

	if(start_cam_flag!=1)
	{
	 I2C_Peripheral_Control(I2C2,ENABLE);
	 for(int i=0;i<1000000;i++);
	 if (OV7670_Init(I2C2) == 0)
	     {
	         // 1. Khởi tạo cấu hình DCMI và DMA
		 for(int i=0;i<1000000;i++);
	         cam_dma_init();
	         dcmi_init();


	         // 2. Clear sạch tất cả các cờ ngắt & cờ lỗi trước đó của DMA2 Stream 1
	         DMA2->LIFCR |= (0x3D << 6);

	         // 3. Clear toàn bộ cờ ngắt DCMI (RAW ISR)
	         DCMI->ICR |= 0x1F;

	         // 4. Bật DCMI Capture
	         DCMI_CAPTRUE(&dcmi, ENABLE);
	     }
	 start_cam_flag=1;
	}
	else  DCMI_CAPTRUE(&dcmi, ENABLE);
	 current_app=CAM_Handlde;
}
void  App_Camera_SNAP_SHOT_Actiton(void)
{
	current_sys_mode = MODE_CAM;
	if(start_cam_flag!=1)
	{
	 I2C_Peripheral_Control(I2C2,ENABLE);
	 for(int i=0;i<1000000;i++);
	 if (OV7670_Init(I2C2) == 0)
	     {
	         // 1. Khởi tạo cấu hình DCMI và DMA
	         cam_dma_init();
	         dcmi_init();


	         // 2. Clear sạch tất cả các cờ ngắt & cờ lỗi trước đó của DMA2 Stream 1
	         DMA2->LIFCR |= (0x3D << 6);

	         // 3. Clear toàn bộ cờ ngắt DCMI (RAW ISR)
	         DCMI->ICR |= 0x1F;

	         // 4. Bật DCMI Capture
	         DCMI_CAPTRUE(&dcmi, ENABLE);
	     }
	 start_cam_flag=1;
	}
	else  DCMI_CAPTRUE(&dcmi, ENABLE);
	 current_app=CAM_SNAP_SHOT_Handlde;
}

// Hàm xử lý Quét SD 1 LẦN DUY NHẤT khi người dùng chọn "SD_MODE"
// Hàm xử lý Quét SD 1 LẦN DUY NHẤT khi người dùng chọn "SD_MODE"
void App_SDMode_Action(void)
{
    // BƯỚC 1: Nếu CHƯA QUÉT THÀNH CÔNG bao giờ -> Tiến hành quét SD
    if (sd_scanned_success == 0)
    {
      static DIR dir;
      static  FILINFO fno;
      static  FRESULT res;

        // Kiểm tra thẻ SD đã mount thành công ở main chưa
        if (!sd_mounted) {
            tft_clear(&spi1, 0x0000);
            tft_write_string(&spi1, 10, 100, "SD NOT MOUNTED!", 0xF800, 0x0000);
            return;
        }

        // In thông báo DUY NHẤT 1 LẦN ngoài vòng lặp (Tránh đụng độ SPI với f_readdir)
        tft_clear(&spi1, 0x0000);
        tft_write_string(&spi1, 20, 100, "Scanning SD...", 0xFFE0, 0x0000);

        // Mở thư mục gốc
        res = f_opendir(&dir, "/");
        if (res != FR_OK) {
            res = f_opendir(&dir, "0:/");
        }

        if (res == FR_OK)
        {
            sd_file_count = 0;

            #if defined(FF_USE_LFN) && (FF_USE_LFN > 0)
            // Cấp bộ đệm đệm LFN nếu dự án bật Long File Name trong ffconf.h
            static char lfn_buffer[64];
            fno.lfname = lfn_buffer;
            fno.lfsize = sizeof(lfn_buffer);
            #endif

            while (sd_file_count < MAX_SD_FILES)
            {
                // TUYỆT ĐỐI KHÔNG gọi tft_write_string ở đây để tránh treo SPI
                res = f_readdir(&dir, &fno);

                // Hết file hoặc lỗi FatFS -> Dừng
                if (res != FR_OK || fno.fname[0] == 0) break;

                // Bỏ qua thư mục, file ẩn, file hệ thống
                if (fno.fname[0] == '.' || (fno.fattrib & (AM_HID | AM_SYS | AM_DIR))) continue;

                // Xác định con trỏ chứa tên file (LFN hoặc Short Name 8.3)
                char *filename_ptr = fno.fname;
                #if defined(FF_USE_LFN) && (FF_USE_LFN > 0)
                if (fno.lfname && fno.lfname[0] != 0) {
                    filename_ptr = fno.lfname;
                }
                #endif

                // Copy tên file vào mảng RAM tĩnh
                strncpy(sd_filenames[sd_file_count], filename_ptr, 19);
                sd_filenames[sd_file_count][19] = '\0';

                // Nối trực tiếp tên file vào làm CON của node SD_MODE (current_node)
                Add_Node(sd_filenames[sd_file_count], current_node, Children, App_PlayVideo_Action);

                sd_file_count++;
            }
            f_closedir(&dir);

            // NẾU TÌM THẤY ÍT NHẤT 1 FILE -> CHỐT CỜ KHÓA VĨNH VIỄN
            if (sd_file_count > 0) {
                sd_scanned_success = 1;
            }
        }

        if (sd_scanned_success == 0) {
            tft_clear(&spi1, 0x0000);
            tft_write_string(&spi1, 20, 100, "No Files Found!", 0xF800, 0x0000);
            return;
        }
    }

    // BƯỚC 2: Nếu đã quét xong (hoặc vừa quét thành công), nhảy thẳng vào danh sách con
    if (current_node->child != NULL) {
        current_node = current_node->child;
        //current_app= Menu_Event;
        Menu_Render(&spi1);
    }
}
// -------------------------------------------------------------
// 2. KHỞI TẠO CÂY MENU
// -------------------------------------------------------------
MenuNode* New_Node(char * title, ActionFunc_t action)
{
    MenuNode *x = (MenuNode*) malloc(sizeof(MenuNode));
    if (x == NULL) return NULL; // Kiểm tra an toàn bộ nhớ Heap
    x->title = title;
    x->parent = NULL;
    x->child = NULL;
    x->sibling_up = NULL;
    x->sibling_down = NULL;
    x->action = action;
    return x;
}

MenuNode *Add_Node(char* title, MenuNode * target_node, int mode, ActionFunc_t action)
{
    MenuNode * new_node = New_Node(title, action);
    if (new_node == NULL || target_node == NULL) return target_node;

    if(mode == Sibling)
    {
        MenuNode* tmp = target_node;
        while(tmp->sibling_down != NULL) tmp = tmp->sibling_down;
        tmp->sibling_down = new_node;
        new_node->sibling_up = tmp;
        new_node->parent = target_node->parent;
    }
    else if(mode == Children)
    {
        if(target_node->child == NULL)
        {
            target_node->child = new_node;
            new_node->parent = target_node;
        }
        else
        {
            MenuNode* tmp = target_node->child;
            while(tmp->sibling_down != NULL) tmp = tmp->sibling_down;
            tmp->sibling_down = new_node;
            new_node->sibling_up = tmp;
            new_node->parent = target_node;
        }
    }
    return target_node;
}

void Menu_Init_Tree(void)
{
    root = New_Node("MAIN MENU", NULL);

    // THAY ĐỔI QUAN TRỌNG:
    // Node SD_MODE sẽ gắn với App_SDMode_Action (Chưa có con nào ở thời điểm Init)
   Add_Node("SD_MODE", root, Children, App_SDMode_Action);
    //Add_Node("ETHERNET MODE", root, Children, App_Ethernet_Action);
    Add_Node("CAM_MODE",root, Children,App_Camera_Actiton);
    Add_Node("CAM_SNAP_SHOT",root->child->sibling_down,Children, App_Camera_SNAP_SHOT_Actiton);
    current_node = root->child;
}

#define ITEMS_PER_PAGE  6   // MỗI trang hiển thị tối đa 6 mục (6 * 30px = 180px, không lo lẹm màn 240px)

void Menu_Render(SPI_Handle_t* hspi)
{
    if (current_node == NULL) return;

    // 1. Tìm node đầu tiên trong danh sách anh em
    MenuNode *first = current_node;
    while (first->sibling_up != NULL) {
        first = first->sibling_up;
    }

    // 2. Tính vị trí (index) của node đang được chọn
    uint8_t current_index = 0;
    MenuNode *idx_tmp = first;
    while (idx_tmp != NULL && idx_tmp != current_node) {
        current_index++;
        idx_tmp = idx_tmp->sibling_down;
    }

    // 3. Tính trang cần hiển thị
    uint8_t target_page = current_index / ITEMS_PER_PAGE;
    // 4. Xóa màn hình DUY NHẤT 1 LẦN trước khi vẽ
    tft_clear(hspi, 0x0000);
    char buff[15];
    sprintf(buff,"PAGE %d",target_page);
    tft_write_string(hspi, 250, 30, buff, 0x0ffff, 0x0000);
    // Vẽ tiêu đề menu cha (nếu có)
    if (current_node->parent != NULL) {
        tft_write_string(hspi, 10, 10, current_node->parent->title, 0x07ff, 0x0000);
    }

    // 5. Duyệt danh sách và CHỈ VẼ các mục thuộc trang target_page
    uint16_t y_pos = 40;
    MenuNode *temp = first;
    uint8_t item_index = 0;

    while (temp != NULL)
    {
        uint8_t item_page = item_index / ITEMS_PER_PAGE;

        // Chỉ vẽ nếu item này thuộc trang hiện tại
        if (item_page == target_page)
        {
            if (temp == current_node) {
                // Đang chọn -> Vẽ dấu mũi tên và tô màu đỏ
                tft_write_string(hspi, 10, y_pos, ">", 0xF800, 0x0000);
                tft_write_string(hspi, 30, y_pos, temp->title, 0xF800, 0x0000);
            } else {
                // Không chọn -> Màu trắng
                tft_write_string(hspi, 30, y_pos, temp->title, 0xFFFF, 0x0000);
            }

            y_pos += 30; // Tăng tọa độ Y cho mục tiếp theo trong trang
        }

        temp = temp->sibling_down;
        item_index++;
    }
}
extern volatile uint16_t adc_results[5];
App_state_t Menu_Event(MenuEvent_t input, SPI_Handle_t *hspi)
{
	Battery_Init(adc_results[0],adc_results[3],&spi1);
    if (current_sys_mode == MODE_SD)
    {
        if (current_sd_app == SD_APP_MENU)
        {
            if (input == EVENT_NONE || current_node == NULL) return;

            switch (input)
            {
                case EVENT_UP:
                    if (current_node->sibling_up != NULL) {
                        current_node = current_node->sibling_up;
                        Menu_Render(hspi);
                    }
                    break;

                case EVENT_DOWN:
                    if (current_node->sibling_down != NULL) {
                        current_node = current_node->sibling_down;
                        Menu_Render(hspi);
                    }
                    break;

                case EVENT_LEFT:
                    if (current_node->child != NULL) {
                        current_node = current_node->child;
                        Menu_Render(hspi);
                    }
                    break;

                case EVENT_RIGHT:
                    if (current_node->parent != NULL) {
                        if (current_node->parent->child != NULL) {
                            current_node = current_node->parent;
                            if (current_node == root) {
                                current_node = root->child;
                            }
                            Menu_Render(hspi);
                        }
                    }
                    break;

                case EVENT_SELECT:
                    if (current_node->action != NULL) {
                        current_node->action(); // Nếu chọn SD_MODE -> Gọi App_SDMode_Action
                        if (current_sd_app == SD_APP_PLAY_RAW) {
                            tft_clear(hspi, 0x0000);
                        }
                    } else if (current_node->child != NULL) {
                        current_node = current_node->child;
                        Menu_Render(hspi);
                    }
                    break;

                default:
                    break;
            }
        }
    }
}
App_state_t ETH_Handle(MenuEvent_t input, SPI_Handle_t *hspi)
{
    if (current_sys_mode == MODE_ETH)
    {
        if (input == EVENT_NONE) {
            ETH_TYPE(&eth);
        }
        if (input == EVENT_RIGHT || input == EVENT_LEFT)
        {
            current_sys_mode = MODE_SD;
            current_sd_app = SD_APP_MENU;
            current_app=Menu_Event;
            tft_clear(hspi, 0x0000);
            Menu_Render(hspi);
            return;
        }
    }
}
App_state_t SD_IMGE_Handle(MenuEvent_t input, SPI_Handle_t *hspi)
{
    if (current_sd_app == SD_APP_PLAY_RAW)
    {
        if (input == EVENT_RIGHT || input == EVENT_LEFT) {
            current_sd_app = SD_APP_MENU;
            read_active = 0;
            current_app=Menu_Event;
            Menu_Render(hspi);
            return;
        }

        if (sd_mounted && read_active) {
            int status = LoadImage_SD_To_TFT_NonBlocking(current_node->title);
            if (status == 1 || status == -1) {
                read_active = 0;
            }
        }
    }
}
extern volatile uint8_t is_capturing,capture_line_count,target_capture_line,capture_buf_ready,cap_flag;
extern FIL capture_file;
extern uint32_t capture_temp_buf[160*16] __attribute__((aligned(4)));
App_state_t CAM_Handlde(MenuEvent_t input, SPI_Handle_t *hspi)
{
	if(current_sys_mode==MODE_CAM)
	{
        if (input == EVENT_RIGHT || input == EVENT_LEFT)
        {
        	DCMI_CAPTRUE(&dcmi, DISABLE);
        	for(int i=0;i<1000000;i++);
            current_sys_mode = MODE_SD;
            current_sd_app = SD_APP_MENU;
            current_app=Menu_Event;
            tft_clear(hspi, 0x0000);
            Menu_Render(hspi);
            return;
        }
	}
}
static void handle_data_capture()
{
    if(is_capturing&& cap_flag==0)
    {
       	for(int i=0;i<10000;i++);

            if (f_open(&capture_file, "CAPTURE.RAW", FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
            {

                capture_line_count = 0;
                target_capture_line = 0; // Reset về dòng đầu tiên
                capture_buf_ready = 0;

            }
            cap_flag=1;
    }
    if (is_capturing && capture_buf_ready)
    {
        UINT bw;

        // XL MÀU SẮC:
        // Nếu ghi RAW để mở lại bằng LoadImage_SD_To_TFT (không swap trong LoadImage):
        // Giữ nguyên đảo byte bswap16 như dưới đây.
        uint16_t *pBuf = (uint16_t*)capture_temp_buf;
        for (uint32_t i = 0; i < 320*16; i++) {
            pBuf[i] = __builtin_bswap16(pBuf[i]);
        }

        // Ghi 1280 Bytes an toàn từ vòng lặp chính (Main loop)
        f_write(&capture_file, capture_temp_buf,640*16, &bw);
        capture_line_count += 16;

        // Xóa cờ để sẵn sàng đón 2 dòng tiếp theo từ ngắt DMA
        capture_buf_ready = 0;

        // Kiểm tra đã đủ 240 dòng chưa
        if (capture_line_count >=240)
        {
            f_close(&capture_file);
            is_capturing = 0;
            capture_line_count=0;
            cap_flag=0;
            tft_clear(&spi1, 0x0000);
            tft_write_string(&spi1, 80, 120, "CAPTURE SUCCESS!", 0x07E0, 0x0000);
        }
    }
}
void receive_data_irq(uint8_t * buffer, uint8_t* temp, uint16_t current_line,uint8_t line)
{
    if (is_capturing)
   {
   	 static uint8_t i=0;
                // Chỉ bắt đúng 2 dòng tương ứng với target_capture_line
                if (current_line == target_capture_line && !capture_buf_ready)
                {
                    // Copy 1280 Bytes ra buffer tạm
                    memcpy((uint8_t*)temp+(i*1280), buffer, 1280);

                    target_capture_line ++; // Chuẩn bị cho 2 dòng tiếp theo

                    if(i==(line-1)/2)
                   	 {
                   	 i=0;
                   	  capture_buf_ready = 1; // Báo cho main() ghi xuống SD

                   	 }
                    else i++;
                }
      }
}
App_state_t CAM_SNAP_SHOT_Handlde(MenuEvent_t input, SPI_Handle_t *hspi)
{
	if(current_sys_mode==MODE_CAM)
	{
        if (input == EVENT_RIGHT || input == EVENT_LEFT)
        {
        	DCMI_CAPTRUE(&dcmi, DISABLE);
        	for(int i=0;i<1000000;i++);
            current_sys_mode = MODE_SD;
            current_sd_app = SD_APP_MENU;
            current_app=Menu_Event;
            tft_clear(hspi, 0x0000);
            Menu_Render(hspi);
            return;
        }
        else if (input == EVENT_SELECT && !is_capturing && sd_mounted)
        {
        	 is_capturing = 1;        // Bật cờ cho phép chụp
        }
        handle_data_capture();
	}
}
void menu_handle(MenuEvent_t input, SPI_Handle_t* hspi)
{
	if(current_app!=NULL)
	{
		current_app(input,hspi);
	}
}
