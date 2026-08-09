/*
 * sdcard.c
 *
 *  Created on: Jul 16, 2026
 *      Author: ADMIN
 */


/*
 * sd.c
 *
 *  Created on: Jul 16, 2026
 *      Author: ADMIN
 */

#include <stdint.h>
#include "stm32f407.h"
#include"gpio.h"
#include"spi.h"
#include<string.h>
#include"uart.h"
#include"i2c.h"
#include"otg.h"
#include"timer.h"
#include"tftcolor.h"
#include "dma.h"
#include "sd.h"
#include "ff.h"
void CLK_init()
{
	    *((__vo uint32_t*)(0x40023C00))&=~(0x7);
		*((__vo uint32_t*)(0x40023C00))|=3;//60-90 mhz for hclk
		//enable hse
		RCC->CR|=1<<16;
		while(!(RCC->CR&(1<<17)));

		//choose PLL entry
		RCC->PLLCFGR|=1<<22;
		//PLL div M
		RCC->PLLCFGR&=~(0x3f);
		RCC->PLLCFGR|=8;//1mhz
		//PLL mul N
		RCC->PLLCFGR&=~(0x1ff<<6);
		RCC->PLLCFGR|=300<<6;//288mhz

		//PLL div q for otg fs 48mhz
		RCC->PLLCFGR&=~(0xf<<24);
		RCC->PLLCFGR|=6<<24;//48mhz
		//PLL div P for sys clock

		RCC->PLLCFGR&=~(0x3<<16);
		RCC->PLLCFGR|=0<<16;//144mhz
		// enable pll
		RCC->CR|=1<<24;
		while(!(RCC->CR&(1<<25)));
		//system clock select
		RCC->CFGR&=~(0x03);
		RCC->CFGR|=2;//pll selected as system clock
		while(((RCC->CFGR>>2)&0x03)!=0x02);//wait system clock sta en pll
		//AHB PRE
		RCC->CFGR&=~(0xf<<4);
		RCC->CFGR|=0<<4;//72mhz
		//APB2
		RCC->CFGR&=~(0x7<<13);
		RCC->CFGR|=0<<13;//36mhz
		//APB1
		RCC->CFGR&=~(0x7<<10);
		RCC->CFGR|=0<<10;//36mhz
}
static inline void FPU_Enable(void)
{
    FPU_CPACR |= (0xF << 20);
    __asm volatile ("dsb 0xF");
    __asm volatile ("isb 0xF");
    FPU->FPCCR |= (1 << 31);   // ASPEN
    FPU->FPCCR |= (1 << 30);   // LSPEN
}
void gpio_init()
{
	GPIO_Handle_t gpio;
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode= GPIO_OUTPUT;
	//gpio.gpioy.mode= GPIO_ALTFUNC;
	//gpio.gpioy.alt_num=GPIO_PIN_AF5;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_9;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpioy.mode= GPIO_ALTFUNC;
	gpio.gpioy.alt_num=GPIO_PIN_AF5;
	gpio.gpioy.pin=GPIO_PIN_10;//clk
	gpio.gpioy.pupd=GPIO_NO_PUPD;
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_3;//sck
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_4;//mói
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_5;//miso
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOC;
	gpio.gpioy.pin=GPIO_PIN_2;//miso
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_3;//MOSi
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode= GPIO_OUTPUT;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_PU;
	gpio.gpioy.pin=GPIO_PIN_6;//command/data
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_7;//rst
	GPIO_Init(&gpio);


	gpio.gpiox=GPIOD;
	gpio.gpioy.mode= GPIO_OUTPUT;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_2;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);

}
SPI_Handle_t spi1;
SPI_Handle_t spi1,spi;
void spi_init()
{
	spi1.spix=SPI2;
	spi1.spiy.baurate=SPI_BAUD_2;
	spi1.spiy.mode=SPI_FULL_DUPLEX;
	spi1.spiy.mode_device=SPI_MASTER_MODE;
	spi1.spiy.data_frame=SPI_8BIT;
	spi1.spiy.frame_fomat=SPI_MSB_FIRST;
	spi1.spiy.CPOL=SPI_CPOL_DI;
	spi1.spiy.CPHA=SPI_CPHA_DI;
	spi1.spiy.SSM=SPI_SSM_EN;
	spi1.spiy.SSI=1;
	//SPI_SSI_ConFig(SPI1,ENABLE);
	SPI_Init(&spi1);
	spi.spix=SPI1;
	spi.spiy.baurate=SPI_BAUD_256;
	spi.spiy.mode=SPI_FULL_DUPLEX;
	spi.spiy.mode_device=SPI_MASTER_MODE;
	spi.spiy.data_frame=SPI_8BIT;
	spi.spiy.frame_fomat=SPI_MSB_FIRST;
	spi.spiy.CPOL=SPI_CPOL_DI;
	spi.spiy.CPHA=SPI_CPHA_DI;
	spi.spiy.SSM=SPI_SSM_EN;
	spi.spiy.SSI=1;
	//SPI_SSI_ConFig(SPI1,ENABLE);
	SPI_Init(&spi);
}
// Thêm khai báo này ở vùng biến toàn cục (phía trên hàm main)
DMA_Handle_t tft_dma;

void tft_dma_init(void)
{
    tft_dma.dmax = DMA1;
    tft_dma.dmay.stream_num = 4;
    tft_dma.dmay.channel = 0;
    tft_dma.dmay.Data_transfer_direction = 1;      // Memory -> Peripheral
    tft_dma.dmay.Memory_mode = ENABLE;             // Tăng địa chỉ nguồn
    tft_dma.dmay.Peripheral_mode = DISABLE;        // Cố định địa chỉ đích (SPI_DR)
    tft_dma.dmay.mem_data_size = 0;                // 8-bit
    tft_dma.dmay.per_data_size = 0;                // 8-bit
    tft_dma.dmay.circular_mode = DISABLE;
    tft_dma.dmay.priority = 3;                     // Very High
    tft_dma.dmay.per_addr = (uint32_t)&(SPI2->DR);
    tft_dma.dmay.direct_mode_dis = DISABLE;
    tft_dma.dmay.Mem_burst_transfer = 0;
    tft_dma.dmay.Peri_burst_transfer = 0;

    // Gọi hàm init (có thể đã có trong thư viện)
    DMA_Init(&tft_dma);

    // 🔥 THÊM: Bật ngắt Transfer Complete (TCIE) cho Stream4
    DMA1->DMA_MEM[4].SCR |= (1 << 4);   // Bit 4 = TCIE
}
void tftt_inti()
{

    gpio_init(); // Gọi hàm cấu hình chân đã sửa ở trên
    spi_init();  // Khởi tạo struct spi1 (đang trỏ tới SPI2)
    //SPI2->CR2 |= (1 << 1);
	//// 1. Nạp cấu hình vào Struct
	//tft_dma_init();

	// 2. MẸO: Chặn không cho DMA tự chạy bằng cách ép số lượng = 0
	//tft_dma.dmay.number_of_data = 0;

	// 3. Gọi Init để ốp toàn bộ cấu hình (Channel, MINC, DIR...) xuống thanh ghi
	// Đến dòng cuối cùng của hàm này, lệnh bật EN sẽ bị STM32 vô hiệu hóa vì NDTR = 0
	//DMA_Init(&tft_dma);

	// 4. Khởi tạo màn hình
	SPI_Control(SPI2, ENABLE);
	tft_init(&spi1);
	tft_clear(&spi1, TFT_COLOR_BLACK);



	//NVIC_ConFig(15,ENABLE);
	//NVIC_PRIORITY(15,1);
}
FATFS fs;           // Vùng làm việc của FatFs cho thẻ nhớ
FIL myFile;         // Biến quản lý File
//FRESULT res;        // Trạng thái trả về
UINT bytesWritten;  // Biến lưu số byte ghi được
UINT bytesRead;          // Biến lưu số byte thực tế đọc được
char read_buffer[100];   // Bộ đệm RAM để chứa nội dung đọc ra từ thẻ nhớ
char write_data[] = "Luu data bang thanh ghi STM32f407 khong dung HAL!\n hahahaha";
static uint8_t run_once = 0;static uint8_t sd_mounted = 0,err=0;
static uint16_t current_line = 0; // Đưa ra ngoài hẳn block đọc file
// Sửa bộ đệm về đúng 512 byte (chuẩn 1 sector phần cứng)
static uint8_t direct_buffer[512] __attribute__((aligned(4)));
// Đã loại bỏ current_line vì không còn cần thiết cho logic vẽ mới

int main(void)
{
    FPU_Enable();
    CLK_init();
    TFT_CS_HIGH();
    tftt_inti();
    SPI_Control(SPI2, ENABLE);
    SPI_Control(SPI1, ENABLE);
    // 2. Khởi tạo thẻ xong, nâng tốc độ SPI lên để vẽ nhanh


    sd_mounted = 0;

    // BẮT BUỘC PHẢI CÓ LỆNH NÀY ĐỂ MỞ ĐƯỢC FILE
    if (f_mount(&fs, "", 1) == FR_OK) {
        sd_mounted = 1;
        SPI_Control(SPI1, DISABLE);
        spi.spiy.baurate=SPI_BAUD_4;
        SPI_Init(&spi);
        SPI_Control(SPI1, ENABLE);
    }

    FRESULT f_res;
    run_once = 0;

    while (1)
    {
        if (run_once == 0 && sd_mounted)
        {

        	f_res = f_open(&myFile, "anh.RAW", FA_READ | FA_OPEN_EXISTING);

            if (f_res == FR_OK)
            {
                // --- THAY ĐỔI QUAN TRỌNG: MỞ WINDOW TOÀN MÀN HÌNH 1 LẦN DUY NHẤT ---
                TFT_CS_LOW();
                tft_set_window(&spi1, 0, 0, 319, 239);
                TFT_CS_HIGH();

                // Vòng lặp đọc liên tục từng block 512 byte
                while (1)
                {
                    // --- BƯỚC A: GIẢI PHÓNG BUS SPI CHO THẺ SD ---
                    TFT_CS_HIGH();

                    while (!(SPI2->SR & (1 << 1)));
                    SPI2->DR = 0xFF;
                    while (!(SPI2->SR & (1 << 0)));
                    volatile uint8_t dump = SPI2->DR;
                    (void)dump;

                    // --- BƯỚC B: ĐỌC ĐÚNG 512 BYTE TỪ THẺ SD ---
                    f_res = f_read(&myFile, direct_buffer, 512, &bytesRead);

                    if (f_res != FR_OK)
                    {
                        err = f_res;
                        f_close(&myFile);
                        break;
                    }

                    if (bytesRead > 0)
                    {
                        // --- BƯỚC C: CHUẨN BỊ BUS CHO TFT ---
                        while (!(SPI2->SR & (1 << 1)));
                        SPI2->DR = 0xFF;
                        while (!(SPI2->SR & (1 << 0)));
                        dump = SPI2->DR;

                        // --- BƯỚC D: ĐẨY DATA RA MÀN HÌNH BẰNG SPI ---
                        TFT_DC_DATA();
                        TFT_CS_LOW();

                        // Bắn mảng 512 byte (hoặc số byte thực tế còn lại ở cuối file) ra SPI
                        SPI_Master_Transmit(spi1.spix, direct_buffer, bytesRead);

                        // Chờ cờ BSY (Busy) hạ xuống 0
                        while ((spi1.spix->SR & (1 << 7)) != 0);

                        // Hút rác thanh ghi DR
                        while (spi1.spix->SR & (1 << 0)) {
                            volatile uint8_t temp = spi1.spix->DR;
                            (void)temp;
                        }
                        volatile uint8_t temp = spi1.spix->SR;
                        temp = spi1.spix->DR;
                        (void)temp;

                        TFT_CS_HIGH();

                        // --- BƯỚC E: KIỂM TRA ĐIỀU KIỆN KẾT THÚC ---
                        // Nếu số byte đọc được ít hơn 512 tức là đã chạm đến byte cuối cùng của file
                        if (bytesRead < 512) {
                        	 f_close(&myFile);
                            break;
                        }
                    }
                    else
                    {
                        err = 2;
                        f_close(&myFile);
                        break; // bytesRead = 0: hết file
                    }
                }

                f_close(&myFile);
                run_once = 1;
            }
            else
            {
                err = f_res;
                run_once = 1;
            }
        }
    }
}
void DMA1_Stream4_IRQHandler(void)
{
    // 1. Kiểm tra cờ TCIF4 (bit 5 trong HISR cho Stream 4)
    if (DMA1->HISR & (1 << 5))
    {
        // 2. Xóa cờ TCIF4 bằng HIFCR (viết 1 vào bit 5 để xóa)
        DMA1->HIFCR = (1 << 5);

        // KHÔNG GỌI TFT_CS_HIGH() VÀ spi_locked = 0 Ở ĐÂY NỮA
        //frame_finished = 1;
    }

    // Xử lý các cờ lỗi cho Stream 4 (nếu cần)
    if (DMA1->HISR & (1 << 3)) { DMA1->HIFCR = (1 << 3); } // CTCIF4 (Transfer Error)
    if (DMA1->HISR & (1 << 2)) { DMA1->HIFCR = (1 << 2); } // CDMEIF4 (Direct Mode Error)
    if (DMA1->HISR & (1 << 0)) { DMA1->HIFCR = (1 << 0); } // CFEIF4 (FIFO Error)
}
