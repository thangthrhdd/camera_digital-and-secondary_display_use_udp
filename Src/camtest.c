/*
 * camtest.c
 *
 *  Created on: Aug 2, 2026
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
#include "dcmi.h"
#include "menu.h"
#include"ov7670.h"
#include "battery.h"
void CLK_init()
{
	// 1. Cấu hình Flash: Set 5 Wait States (bắt buộc cho 168MHz)
	    //    + Bật Prefetch, Instruction Cache, Data Cache để CPU không bị nghẽn
	    *((__vo uint32_t*)(0x40023C00)) = (5 << 0)  | // LATENCY = 5 WS
	                                     (1 << 8)  | // PRFTEN: Prefetch Enable
	                                     (1 << 9)  | // ICEN: Instruction Cache Enable
	                                     (1 << 10);  // DCEN: Data Cache Enable

	    // 2. Enable HSE (Thạch anh ngoài 8MHz)
	    RCC->CR |= (1 << 16);
	    while (!(RCC->CR & (1 << 17)));

	    // 3. Cấu hình MCO2 (PC9) xuất HSE làm XCLK (8MHz) cho Camera
	    RCC->CFGR &= ~(0x1F << 27);
	    RCC->CFGR |=  (0x06  << 27); // Select HSE as MCO2 source

	    // 4. Cấu hình PLL: SYSCLK = 168MHz, USB OTG = 48MHz
	    // Công thức: VCO_in = 8 / 8 = 1MHz -> VCO_out = 1 * 336 = 336MHz
	    //            SYSCLK = 336 / 2 = 168MHz | USB = 336 / 7 = 48MHz
	    RCC->PLLCFGR = (8 << 0)   | // M = 8
	                   (336 << 6) | // N = 336
	                   (0 << 16)  | // P = 2 (00 tương ứng div 2)
	                   (1 << 22)  | // PLLSRC = HSE
	                   (7 << 24);   // Q = 7

	    // 5. Enable PLL
	    RCC->CR |= (1 << 24);
	    while (!(RCC->CR & (1 << 25)));

	    // 6. Prescalers KỊCH KHUNG:
	    // AHB  = 168MHz (/1)
	    // APB1 = 42MHz  (/4) -> Mức MAX giới hạn của bus APB1 (I2C, SPI2/3)
	    // APB2 = 84MHz  (/2) -> Mức MAX giới hạn của bus APB2 (DCMI, SPI1)
	    RCC->CFGR &= ~((0xF << 4) | (0x7 << 10) | (0x7 << 13));
	    RCC->CFGR |= (0 << 4);   // AHB  = 168MHz (/1)
	    RCC->CFGR |= (5 << 10);  // APB1 = 42MHz  (/4)
	    RCC->CFGR |= (4 << 13);  // APB2 = 84MHz  (/2)

	    // 7. Select PLL làm System Clock
	    RCC->CFGR &= ~(0x03);
	    RCC->CFGR |= 2;
	    while (((RCC->CFGR >> 2) & 0x03) != 0x02);
}
static inline void FPU_Enable(void)
{
    FPU_CPACR |= (0xF << 20);
    __asm volatile ("dsb 0xF");
    __asm volatile ("isb 0xF");
    FPU->FPCCR |= (1 << 31);   // ASPEN
    FPU->FPCCR |= (1 << 30);   // LSPEN
}
DMA_Handle_t cam_dma,tft_dma,adc_dma;
#define FRAME_WIDTH   320
#define FRAME_HEIGHT  240
#define FRAME_BUFFER_SIZE_WORDS ((FRAME_WIDTH * FRAME_HEIGHT * 2) / 4)
uint32_t frame_buffer[320]__attribute__((aligned(4)));
void cam_dma_init(void)
{
    cam_dma.dmax = DMA2;
    cam_dma.dmay.stream_num = 1;
    cam_dma.dmay.channel = 1;

    // 1. Chuyển chiều truyền từ DCMI (Peripheral) vào RAM (Memory)
    cam_dma.dmay.Data_transfer_direction = 0;      // 0: Peripheral -> Memory

    // 2. Tăng địa chỉ RAM để ghi tiếp vào ô nhớ sau mỗi lần chuyển 32-bit
    cam_dma.dmay.Memory_mode = ENABLE;             // Tăng địa chỉ đích (RAM)
    cam_dma.dmay.Peripheral_mode = DISABLE;        // Cố định địa chỉ nguồn (&DCMI->DR)

    // 3. DCMI FIFO bắt buộc phải đọc/ghi theo Word (32-bit)
    cam_dma.dmay.mem_data_size = 2;                // 2: 32-bit (Word)
    cam_dma.dmay.per_data_size = 2;                // 2: 32-bit (Word)

    // 4. Bật Circular Mode để DMA tự động nhận lại dữ liệu khung hình tiếp theo
    cam_dma.dmay.circular_mode = ENABLE;           // [SỬA]: Chuyển sang ENABLE
    cam_dma.dmay.priority = 3;                     // Very High (Tốt nhất cho DCMI)

    // 5. Địa chỉ nguồn & đích
    cam_dma.dmay.per_addr = (uint32_t)&(DCMI->DR);
    cam_dma.dmay.mem0_addr = (uint32_t)frame_buffer; // [SỬA]: Ép kiểu uint32_t

    // 6. Số lượng Word cần truyền (Gán đúng bằng kích thước buffer)
    cam_dma.dmay.number_of_data =320; // [SỬA]: 1284 Words

    // 7. [CỰC KỲ QUAN TRỌNG]: Bắt buộc BẬT FIFO MODE cho DCMI!
    cam_dma.dmay.direct_mode_dis = ENABLE;         // [SỬA]: ENABLE để bật FIFO (bit 2 = 1)
    cam_dma.dmay.FIFO_threshold = 3;               // Full FIFO (4 Words)

    // 8. Single transfer
    cam_dma.dmay.Mem_burst_transfer = 0;
    cam_dma.dmay.Peri_burst_transfer = 0;

    DMA_Init(&cam_dma);
    DMA2->DMA_MEM[1].SCR |= (1 << 4);
}
void tft_dma_init(void)
{
    tft_dma.dmax = DMA1;
    tft_dma.dmay.stream_num = 5;
    tft_dma.dmay.channel = 0;
    tft_dma.dmay.Data_transfer_direction = 1;      // Memory -> Peripheral
    tft_dma.dmay.Memory_mode = ENABLE;             // Tăng địa chỉ nguồn
    tft_dma.dmay.Peripheral_mode = DISABLE;        // Cố định địa chỉ đích (SPI_DR)
    tft_dma.dmay.mem_data_size = 0;                // 8-bit
    tft_dma.dmay.per_data_size = 0;                // 8-bit
    tft_dma.dmay.circular_mode = DISABLE;
    tft_dma.dmay.priority = 3;                     // Very High
    tft_dma.dmay.per_addr = (uint32_t)&(SPI3->DR);
    tft_dma.dmay.direct_mode_dis = DISABLE;
    tft_dma.dmay.Mem_burst_transfer = 0;
    tft_dma.dmay.Peri_burst_transfer = 0;

    // Gọi hàm init (có thể đã có trong thư viện)
    DMA_Init(&tft_dma);
    //  THÊM: Bật ngắt Transfer Complete (TCIE) cho Stream4
    DMA1->DMA_MEM[5].SCR |= (1 << 4);   // Bit 4 = TCIE
    // ADC1 bắt buộc dùng DMA2 Stream 0, Channel 0
}
DCMI_Handle_t dcmi;
void dcmi_init()
{
	dcmi.dcmix=DCMI;
	dcmi.dcmiy.data_mode= DCMI_8BIT ;
	dcmi.dcmiy.Frame_capture_rate=DCMI_All_frames;
	dcmi.dcmiy.Vertical_syn_polarity=DCMI_VSYN_HIGH;
	dcmi.dcmiy.Horizontal_syn_polarity=DCMI_HSYN_LOW;
	dcmi.dcmiy.Pixel_clock_polarity=DCMI_PIXEL_RISING;
	dcmi.dcmiy.hard_or_embed_mode=DMCI_HARD_MODE;
	dcmi.dcmiy.JPEG_format=DCMI_Uncompressed;
	dcmi.dcmiy.Crop_feature=DMCI_CROP ;
	dcmi.dcmiy.captrue_mode=DMCI_Continuous_grab;
	DCMI_Init(&dcmi);
	DCMI->CWSTRT = 0x00000000;
	    // CWSIZE: VLINE (số dòng) = 240, CAPCNT (số byte mỗi dòng / 2 - 1 hoặc số pixel)
	    // 320 pixels RGB565 = 640 bytes -> CAPCNT = 319 (0x013F), VLINE = 239 (0x00EF)
	    DCMI->CWSIZE = (239 << 16) | (639 << 0);
	DCMI->IER|=1<<3;
}
I2C_Handle_t i2c;
void i2c_init()
{
	i2c.i2cx=I2C2;
	i2c.i2cy.ACK_control=I2C_ACK_EN;
	i2c.i2cy.addr_device=0x01;
	i2c.i2cy.speed=I2C_SM_100Khz;
	i2c.i2cy.duty_cycle=I2C_SM_MODE;
	I2C_Init(&i2c);
}
void gpio_init()
{
	GPIO_Handle_t gpio;
	gpio.gpiox=GPIOA;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_4;//DCMI_ HSYNC
	gpio.gpioy.alt_num=GPIO_PIN_AF13;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_6;//DCMI_ PIXCLK
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_9;// DCMI_D0
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_10;// DCMI_D1
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOB;
	gpio.gpioy.pin=GPIO_PIN_6;// DCMI_D5
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_7;// DCMI_VSYN_C
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_8;// DCMI_D6
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_9;// DCMI_D7
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOC;
	gpio.gpioy.pin=GPIO_PIN_8;// DCMI_D2
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_11;// DCMI_D4
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_OPDR;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_10;//scl
	gpio.gpioy.alt_num=GPIO_PIN_AF4;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_11;
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOE;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_1; //DCMI_D3
	gpio.gpioy.alt_num=GPIO_PIN_AF13;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);

	gpio.gpiox=GPIOC;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_9;//XCLK
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOD;
	gpio.gpioy.mode= GPIO_OUTPUT;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_2;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpioy.mode= EXTI_RTFT;
	gpio.gpioy.pin=GPIO_PIN_0;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	NVIC_ConFig(6,ENABLE);
	NVIC_PRIORITY(6,1);
}
SPI_Handle_t spi1,spi;
void gpio_spi()
{
	GPIO_Handle_t gpio;
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_3;//sck
	gpio.gpioy.alt_num=GPIO_PIN_AF6;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_4;//mosi
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_5;//miso

	GPIO_Init(&gpio);
	gpio.gpiox=GPIOD;
	gpio.gpioy.mode=GPIO_OUTPUT;
	gpio.gpioy.pin=GPIO_PIN_5;//nss
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_6;//dc
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_7;//rst
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode=GPIO_ALTFUNC;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_13;//sck
	gpio.gpioy.alt_num=GPIO_PIN_AF5;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOC;
	gpio.gpioy.pin=GPIO_PIN_2;//MISO
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_3;//Mosi
	GPIO_Init(&gpio);
	gpio.gpiox=GPIOB;
	gpio.gpioy.mode=GPIO_OUTPUT;
	gpio.gpioy.pin=GPIO_PIN_12;//nss
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	GPIO_Init(&gpio);
	spi1.spix=SPI3;
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

	spi.spix=SPI2;
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
volatile uint16_t adc_results[5];
void adc_dma_init()
{
    adc_dma.dmax = DMA2;
    adc_dma.dmay.stream_num = 0;
    adc_dma.dmay.channel = 0;

    adc_dma.dmay.Data_transfer_direction = 0;      // 0: Peripheral -> Memory
    adc_dma.dmay.Memory_mode = ENABLE;             // Tăng địa chỉ đích (mảng adc_results)
    adc_dma.dmay.Peripheral_mode = DISABLE;        // Cố định địa chỉ nguồn (&ADC1->DR)

    adc_dma.dmay.mem_data_size = 1;                // 1: 16-bit (Half-word)
    adc_dma.dmay.per_data_size = 1;                // 1: 16-bit (Half-word)

    adc_dma.dmay.circular_mode = ENABLE;           // ENABLE: Tự động lặp lại liên tục
    adc_dma.dmay.priority = 3;                     // High priority

    adc_dma.dmay.per_addr = (uint32_t)&(ADC1->DR);  // Địa chỉ nguồn: ADC1 Data Register
    adc_dma.dmay.mem0_addr = (uint32_t)adc_results;  // Địa chỉ đích: Mảng RAM 3 phần tử
    adc_dma.dmay.number_of_data = 4;               // Chuyển đúng 3 kênh (Rank 1, 2, 3)

    adc_dma.dmay.direct_mode_dis = DISABLE;
    adc_dma.dmay.Mem_burst_transfer = 0;
    adc_dma.dmay.Peri_burst_transfer = 0;

    // Bật Clock DMA2 trước khi Init (nếu trong thư viện DMA_Init chưa bật)
    RCC->AHB1ENR |= (1 << 22); // Bit 22: DMA2 clock enable

    // Call hàm Init của bạn
    DMA_Init(&adc_dma);

    // Bật DMA Stream 0 chạy ngay
    DMA2->DMA_MEM[0].SCR |= (1 << 0); // Bit 0 = EN

}
ADC_Handle_t adc1;
void adc_init()
{
    // 1. Cấu hình các kênh ADC (Channel & Rank)
    adc1.adcx = ADC1;
    adc1.adcy.length_of_cvst = 3; // Quét 3 kênh (Length = N - 1 = 2)
    adc1.adcy.resolution = 0;     // 12-bit
    adc1.adcy.cycle = 7;          // Sample time

    adc1.adcy.channel = 8;
    adc1.adcy.rank = 3;
    ADC_Init(&adc1);

    adc1.adcy.channel = 2;
    adc1.adcy.rank = 2;
    ADC_Init(&adc1);

    adc1.adcy.channel = 9;
    adc1.adcy.rank = 1;
    ADC_Init(&adc1);
    adc1.adcy.channel=17;
    adc1.adcy.rank = 4;
    ADC_Init(&adc1);
    ADC1_PCLK_EN();
    *(volatile  uint32_t*)(ADC_BASEADDR+0x300+0x04)|= 1<<23;
    // 2. Cấu hình GPIO Analog (PB0 - Ch8, PA3 - Ch3, PA4 - Ch4)
    GPIO_Handle_t x;
    x.gpiox = GPIOB;
    x.gpioy.mode = GPIO_ANALOG_MODE;
    x.gpioy.optype = GPIO_OUTPUT_PP;
    x.gpioy.ospeed = GPIO_VRY_HIGH_SPEED;
    x.gpioy.pin = 0;
    x.gpioy.alt_num = GPIO_PIN_AF0;
    x.gpioy.pupd = GPIO_NO_PUPD;
    GPIO_Init(&x);
    x.gpioy.pin = 1;
    GPIO_Init(&x);

    x.gpiox = GPIOA;
    x.gpioy.pin = 2;
    GPIO_Init(&x);


    // 3. Khởi tạo DMA2 Stream 0 trước khi trigger ADC
    adc_dma_init();

    // 4. Cấu hình thanh ghi ADC1 cho DMA
    ADC1->CR1 |= (1 << 8);  // SCAN Mode (Bật quét chuỗi nhiều kênh)
    ADC1->CR1 &= ~(1 << 5); // TẮT EOCIE (Không cần ngắt EOC nữa, DMA tự lo)

    ADC1->CR2 |= (1 << 1);  // CONT (Continuous conversion - Tự quét liên tục)
    ADC1->CR2 |= (1 << 8);  // DMA Enable (Cho phép gửi YC DMA)
    ADC1->CR2 |= (1 << 9);  // DDS (DMA Requests continuous - Giữ YC DMA liên tục)

    // 5. Kích hoạt ADC & Bắt đầu chuyển đổi (SWSTART)
    ADC1->CR2 |= (1 << 0);  // ADON lần 1: Bật nguồn ADC
    for (volatile int i = 0; i < 10000; i++); // Chờ ADC ổn định
    ADC1->CR2 |= (1 << 0);  // ADON lần 2
    ADC1->CR2 |= (1 << 30); // SWSTART: Kích hoạt chuỗi chuyển đổi đầu tiên

    // Không cần cấu hình NVIC_ConFig(18, ENABLE) hay NVIC_PRIORITY cho ADC nữa!
}
//uint8_t id;
volatile uint8_t frame_finished=0;
volatile TFT_Line_Element_t active_line;
volatile uint16_t current_line = 0;
FATFS fs;           // Vùng làm việc của FatFs cho thẻ nhớ
FIL myFile;         // Biến quản lý File
FRESULT res;        // Trạng thái trả về
volatile uint8_t sd_mounted = 1,read_active=0;

///captrue
volatile uint8_t is_capturing,capture_line_count,target_capture_line,capture_buf_ready,cap_flag=0;
uint32_t capture_temp_buf[160*16] __attribute__((aligned(4)));
FIL capture_file;
int main(void)
{
 	 CLK_init();
 	FPU_Enable();
	 gpio_init();
	 adc_init();
	 i2c_init();
	 tft_dma_init();
	 gpio_spi();
	 SPI3->CR2|=1<<1;//dma tx buffer
	 SPI_Control(SPI2, ENABLE);
	 SPI_Control(SPI3, ENABLE);
	 for(int i=0;i<100000;i++);
	 TFT_CS_HIGH();
	 tft_init(&spi1);
	 tft_clear(&spi1, TFT_COLOR_BLACK);
	  //OV7670_Read_Reg(I2C2, 0x0A, &id);
		CS_SET();
	    sd_mounted = 0;
	    if (f_mount(&fs, "", 1) == FR_OK) {
	        sd_mounted = 1;
	        SPI_Control(SPI2, DISABLE);
	        spi.spiy.baurate = SPI_BAUD_4;
	        SPI_Init(&spi);
	        SPI_Control(SPI2, ENABLE);
	    }

	    Menu_Init_Tree();
	    Menu_Render(&spi1);
		NVIC_ConFig(16,ENABLE);
		NVIC_PRIORITY(16,1);
		NVIC_ConFig(57,ENABLE);
		NVIC_PRIORITY(57,1);
		NVIC_ConFig(78,ENABLE);
		NVIC_PRIORITY(78,1);

	 while(1)
	 {

	        MenuEvent_t joy_event = adc_handle((uint16_t*)adc_results);

	        // B. Xử lý Logic Menu / Đọc file SD theo Title / Xử lý Ethernet Packet
	        menu_handle(joy_event, &spi1);

		 if (TFT_RingBuffer_Pop((TFT_Line_Element_t*)&active_line))
		         {
		             // [ĐÃ SỬA]: `line_id` đã là tọa độ Y chuẩn (0, 2, 4,...), KHÔNG nhân 2 ở đây!
		             uint16_t start_y = active_line.line_id*2;

		             if (start_y <= 238)
		             {
		                 // Cửa sổ 2 dòng (start_y đến start_y + 1)
		                 tft_set_window(&spi1, 0, start_y,319, start_y + 1);

		                 TFT_DC_DATA();
		                 TFT_CS_LOW();

		                 // Tắt DMA2 Stream 3 để cấu hình
		                 DMA1->DMA_MEM[5].SCR &= ~(1 << 0);
		                 while (DMA1->DMA_MEM[5].SCR & (1 << 0));

		                 // [ĐÃ SỬA]: Clear sạch cờ ngắt cho DMA2 Stream 3 (Dịch 22 bits)
		                 DMA1->HIFCR |= (0x3D << 6);

		                 // Nạp địa chỉ RAM & Số byte truyền
		                 DMA1->DMA_MEM[5].SM0AR = (uint32_t)active_line.color_data;
		                 DMA1->DMA_MEM[5].SNDTR = 1280;

		                 // Bật DMA2 Stream 3
		                 DMA1->DMA_MEM[5].SCR |= (1 << 0);

		                 // Chờ DMA chuyển xong 1280 Bytes
		                 while (DMA1->DMA_MEM[5].SNDTR > 0);

		                 // Chờ SPI1 hết bận (BSY bit = 7)
		                 while (SPI3->SR & (1 << 7));

		                 // Đọc dọn dẹp SPI1 DR/SR
		                 volatile uint8_t temp;
		                 while (SPI3->SR & (1 << 0)) { temp = SPI3->DR; }
		                 temp = SPI3->SR;
		                 temp = SPI3->DR;
		                 (void)temp;

		                 TFT_CS_HIGH();
		             }
		         }

	        if (frame_finished) {
	            frame_finished = 0;
	            //ADC1->CR2|=1<<30;
	        }

	 }
}
void DMA1_Stream5_IRQHandler(void)
{
    // 1. Kiểm tra cờ TCIF4 (bit 5 trong HISR cho Stream 4)
    if (DMA1->HISR & (1 << 11))
    {
        // 2. Xóa cờ TCIF4 bằng HIFCR (viết 1 vào bit 5 để xóa)
        DMA1->HIFCR |= (1 << 11);

        // KHÔNG GỌI TFT_CS_HIGH() VÀ spi_locked = 0 Ở ĐÂY NỮA
        frame_finished = 1;
    }
}
void DMA2_Stream1_IRQHandler(void)
{
	 if (DMA2->LISR & (1 << 11)) // Kiểm tra cờ TCIF1
	 {
		 receive_data_irq(frame_buffer,capture_temp_buf,current_line,16);
		 if(TFT_RingBuffer_IsFull()!=1)
		 {
		 uint16_t *pBuf = (uint16_t*)frame_buffer;
		             for (uint32_t i = 0; i < (640); i++) {
		                 pBuf[i] = __builtin_bswap16(pBuf[i]);
		             }

		    		  TFT_RingBuffer_Push(current_line,(uint8_t*)frame_buffer);
		 }
		 else
		 {

		 }
	     if(current_line<240)
	     current_line++;
	     //else current_line=0;
	     // 3. Xóa cờ ngắt DMA
	     DMA2->LIFCR |= (1 << 11);


	 }
}
void DCMI_IRQHandler()
{
    // 1. Xử lý ngắt VSYNC (Bắt đầu khung hình mới) [11]
    if(DCMI->MIS & (1 << 3))
    {
        DCMI->ICR |= (1 << 3); // Xóa cờ VSYNC
        current_line = 0;      // Reset biến đếm dòng

        // 🔥 QUAN TRỌNG: Reset DMA2 Stream 1 để con trỏ ghi quay về byte 0
        DMA2->DMA_MEM[1].SCR &= ~(1 << 0); // Tắt DMA
        while( DMA2->DMA_MEM[1].SCR & (1 << 0)); // Đợi DMA dừng hẳn [14]

        DMA2->DMA_MEM[1].SNDTR= 320; // Nạp lại 320 Word (2 dòng QVGA) [15]

        // Xóa sạch các cờ lỗi cũ của DMA2 Stream 1 để tránh bị treo
        DMA2->LIFCR |= (0x3D << 6);

        DMA2->DMA_MEM[1].SCR |= (1 << 0); // Bật lại DMA
    }

    // 2. Xử lý lỗi Overrun (OVR) [16]
    if(DCMI->MIS & (1 << 1))
    {
        DCMI->ICR |= (1 << 1);
    }
}
void EXTI0_IRQHandler ()
{
	if(GPIO_ReadPin(GPIOD,GPIO_PIN_0)==1)
	adc_results[4]=EVENT_NONE;
	else adc_results[4]=EVENT_SELECT;
	GPIO_IRQ_Handle(0);
}
