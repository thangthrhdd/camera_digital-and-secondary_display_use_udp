/*
 * tft.c
 *
 *  Created on: Jul 4, 2026
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
//#include"imge.h"
#include "dma.h"
////khai bao usb cdc-----------------------------------/////////////
OTG_FS_ConFig_t g;
uint32_t packet[64];
USB_SETUP_PACKET op,pp;
uint8_t mode,pktsts=0,count=0,count1=0;
uint16_t bcnt=0,addr,addr1;
char bufer[11];
uint8_t set_line[7];
uint8_t receive[64];
uint8_t key=0,rei=0;
///////////////////////////////////

extern uint8_t rx_buf[6400];
extern volatile uint8_t rx_ready;
extern volatile uint8_t* main_draw_ptr;

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
	RCC->PLLCFGR|=288<<6;//288mhz

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
	RCC->CFGR|=4<<13;//36mhz
	//APB1
	RCC->CFGR&=~(0x7<<10);
	RCC->CFGR|=4<<10;//144hz

	GPIO_Handle_t x;
	x.gpiox=GPIOA;
	x.gpioy.mode= GPIO_ALTFUNC;
	x.gpioy.optype=GPIO_OUTPUT_PP;
	x.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	x.gpioy.pin=GPIO_PIN_8;
	x.gpioy.alt_num=GPIO_PIN_AF10;
	//x.gpioy.pupd=GPIO_PU;
	GPIO_Init(&x);
	x.gpioy.pin=GPIO_PIN_9;
	GPIO_Init(&x);
	x.gpioy.pin=GPIO_PIN_10;
	GPIO_Init(&x);
	x.gpioy.pin=GPIO_PIN_11;
	GPIO_Init(&x);
	x.gpioy.pin=GPIO_PIN_12;
	GPIO_Init(&x);
	g.Global= OTG_GLOBAL;
	g.Host=OTG_HOST;
	g.Device=OTG_DEVICE;
	g.PCGCCTL=OTG_POWER;
	OTG_FS_Core_Init(&g);
	OTG_FS_Device_Init(&g);
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
	gpio.gpiox=GPIOA;
	gpio.gpioy.mode= GPIO_OUTPUT;
	//gpio.gpioy.mode= GPIO_ALTFUNC;
	//gpio.gpioy.alt_num=GPIO_PIN_AF5;
	gpio.gpioy.optype=GPIO_OUTPUT_PP;
	gpio.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	gpio.gpioy.pin=GPIO_PIN_4;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pupd=GPIO_NO_PUPD;
	GPIO_Init(&gpio);
	gpio.gpioy.mode= GPIO_ALTFUNC;
	gpio.gpioy.alt_num=GPIO_PIN_AF5;
	gpio.gpioy.pin=GPIO_PIN_5;//clk
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_6;//miso
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_7;//MOSi
	GPIO_Init(&gpio);
	gpio.gpioy.mode= GPIO_OUTPUT;
	gpio.gpioy.alt_num=GPIO_PIN_AF0;
	gpio.gpioy.pin=GPIO_PIN_0;//command/data
	GPIO_Init(&gpio);
	gpio.gpioy.pin=GPIO_PIN_1;//rst
	GPIO_Init(&gpio);
}
// Thêm khai báo này ở vùng biến toàn cục (phía trên hàm main)
DMA_Handle_t tft_dma;

void tft_dma_init(void)
{
    tft_dma.dmax = DMA2; // SPI1 nằm trên DMA2

    // Cấu hình luồng truyền (Mặc định SPI1_TX là Stream 3, Channel 3 trên STM32F401)
    tft_dma.dmay.stream_num = 3;
    tft_dma.dmay.channel = 3;

    tft_dma.dmay.Data_transfer_direction = 1; // 1: Memory-to-peripheral
    tft_dma.dmay.Memory_mode = ENABLE;         // Tự động tăng địa chỉ nguồn (RAM)
    tft_dma.dmay.Peripheral_mode = DISABLE;    // Cố định địa chỉ đích (Thanh ghi SPI_DR)

    tft_dma.dmay.mem_data_size = 0; // 00: 8-bit
    tft_dma.dmay.per_data_size = 0; // 00: 8-bit
    tft_dma.dmay.circular_mode = DISABLE; // Không lặp (Truyền xong 1 khối 10 dòng thì dừng)
    tft_dma.dmay.priority = 3;      // Very High priority để ưu tiên hiển thị màn hình

    // Địa chỉ ngoại vi cố định
    tft_dma.dmay.per_addr = (uint32_t)&(SPI1->DR);

    // Direct mode để tối ưu tốc độ cấu hình đơn giản
    tft_dma.dmay.direct_mode_dis = DISABLE;
    tft_dma.dmay.Mem_burst_transfer = 0;
    tft_dma.dmay.Peri_burst_transfer = 0;
}
SPI_Handle_t spi1;
void spi_init()
{
	spi1.spix=SPI1;
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
}
uint8_t tft_tx_buf[6400];
int main(void)
{
	FPU_Enable();
	CLK_init();
	gpio_init();
	spi_init();

	// 1. Nạp cấu hình vào Struct
	tft_dma_init();

	// 2. MẸO: Chặn không cho DMA tự chạy bằng cách ép số lượng = 0
	tft_dma.dmay.number_of_data = 0;

	// 3. Gọi Init để ốp toàn bộ cấu hình (Channel, MINC, DIR...) xuống thanh ghi
	// Đến dòng cuối cùng của hàm này, lệnh bật EN sẽ bị STM32 vô hiệu hóa vì NDTR = 0
	DMA_Init(&tft_dma);

	// 4. Khởi tạo màn hình
	SPI_Control(SPI1, ENABLE);
	tft_init(&spi1);
	tft_clear(&spi1, TFT_COLOR_BLACK);

	// 5. Cho phép SPI1 phát tín hiệu gọi DMA
	SPI1->CR2 |= (1 << 1);

	uint16_t current_row = 0;

	while(1)
	{
		if(rx_ready)
		{
			rx_ready = 0;

			tft_set_window(&spi1, 0, current_row, 320 - 1, current_row + 10 - 1);

			GPIO_WritePin(GPIOA, GPIO_PIN_0, SET);   // DC = 1
			GPIO_WritePin(GPIOA, GPIO_PIN_4, RESET); // CS = 0

			// ========================================================
			// Dùng lại đúng đoạn code tối ưu tốc độ của bạn!
			// Lần này DMA đã có sẵn cấu hình ngầm từ lúc setup.
			// ========================================================

			// 1. Đảm bảo Stream đã tắt hoàn toàn
			tft_dma.dmax->DMA_MEM[3].SCR &= ~(1 << 0);
			while(tft_dma.dmax->DMA_MEM[3].SCR & (1 << 0));

			// 2. Xóa cờ ngắt cũ (Mask 0x3D là chuẩn xác tuyệt đối cho Stream 3)
			tft_dma.dmax->LIFCR |= (0x3D << 22);

			// 3. Nạp địa chỉ ảnh mới và độ dài
			tft_dma.dmax->DMA_MEM[3].SM0AR = (uint32_t)main_draw_ptr;
			tft_dma.dmax->DMA_MEM[3].SNDTR = 6400;

			// 4. Khai hỏa DMA
			tft_dma.dmax->DMA_MEM[3].SCR |= (1 << 0);

			// 5. Chờ DMA truyền xong vào thanh ghi SPI
			while ( !((tft_dma.dmax->LISR) & (1 << 27)) );
			tft_dma.dmax->LIFCR |= (1 << 27); // Xóa cờ TCIF3 ngay lập tức

			// 6. Đợi SPI đẩy nốt bit cuối trên dây ra màn hình
			while((SPI1->SR & (1 << 7)) != 0);

			GPIO_WritePin(GPIOA, GPIO_PIN_4, SET);   // CS = 1
			// ========================================================

			current_row += 10;
			if(current_row >= 240)
			{
				current_row = 0;
			}
		}
	}
}
void OTG_FS_IRQHandler ()
{
	OTG_IRQ_Handle(&g);
}
