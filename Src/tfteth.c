
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
//#include "ui.h"
#include "menu.h"
uint8_t o;
int k=0;
SPI_Handle_t y;
UART_Handle_t z;
I2C_Handle_t t;
OTG_FS_ConFig_t g;
uint32_t packet[64];
USB_SETUP_PACKET op,pp;
uint8_t mode,pktsts=0,count=0,count1=0;
uint16_t bcnt=0,addr,addr1;
char bufer[11];
uint8_t set_line[7];
uint8_t receive[64];
TIM_Handle_t tim1;
ETH_Handle_t eth;
ADC_Handle_t adc1;
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
void ETH_PIN_Init()
{
	GPIO_Handle_t y;
	y.gpiox=GPIOA;
	y.gpioy.mode=GPIO_ALTFUNC;
	//y.gpioy.mode=GPIO_OUTPUT;
	y.gpioy.optype=GPIO_OUTPUT_PP;
	y.gpioy.ospeed=GPIO_VRY_HIGH_SPEED;
	y.gpioy.pin=GPIO_PIN_1;//ref_clk
	y.gpioy.alt_num= GPIO_PIN_AF11;
	//y.gpioy.alt_num= GPIO_PIN_AF0;
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_2;//MDIO
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_7;// CRS
	GPIO_Init(&y);
	y.gpiox=GPIOB;
	y.gpioy.pin=GPIO_PIN_11;//txen
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_12;//txd0
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_13;//txd1
	GPIO_Init(&y);
	y.gpiox=GPIOC;
	//y.gpioy.mode=GPIO_OUTPUT;
	y.gpioy.pin=GPIO_PIN_4;//rxd0
	//y.gpioy.alt_num= GPIO_PIN_AF0;
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_5;//rxd1
	GPIO_Init(&y);
	y.gpioy.pin=GPIO_PIN_1;//mdc
	GPIO_Init(&y);
}
ETH_Handle_t eth;
uint16_t id2,id3,id1;
volatile uint8_t tx_buffer[1328]__attribute__((aligned(4))) ;
volatile uint8_t rx_buffer[1328]__attribute__((aligned(4))) ;
uint32_t test=0;
void config_tx_buffer(ETH_DMADescTypeDef * txbuf, uint32_t buffer_addr, uint32_t next_desc_addr)
{
    txbuf->Buffer1Addr = buffer_addr;           // Word 2: Trỏ tới data thực tế
    txbuf->Buffer2NextDescAddr = next_desc_addr; // Word 3: Trỏ tới Descriptor kế tiếp

    // Word 1: Quy định kích thước và bật tính năng Chained (bit 20)
    txbuf->ControlBufferSize = 1328 | (1UL << 20);

    // Word 0: Nhượng quyền cho DMA
    txbuf->Status |= (1UL << 31) | (1UL << 29) | (1UL << 28);
}
void config_rx_buffer(ETH_DMADescTypeDef * rxbuf, uint32_t buffer_addr, uint32_t next_desc_addr)
{
    rxbuf->Buffer1Addr = buffer_addr;           // Word 2: Vùng RAM để DMA ghi gói tin vào
    rxbuf->Buffer2NextDescAddr = next_desc_addr; // Word 3: Trỏ tới Descriptor tiếp theo (hoặc chính nó)

    // Word 1: Buffer size 1536 và bật bit RCH (bit 14) cho Second Address Chained
    rxbuf->ControlBufferSize = 1328 | (1UL << 14);

    // Word 0: BẮT BUỘC set bit OWN (bit 31) = 1 cho Rx ngay từ đầu
    rxbuf->Status = (1UL << 31);
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
	gpio.gpioy.pupd=GPIO_PU;
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
	gpio.gpioy.mode= EXTI_RTFT;
	gpio.gpioy.pin=GPIO_PIN_1;
	gpio.gpioy.pupd=GPIO_PU;
	GPIO_Init(&gpio);
	NVIC_ConFig(7,ENABLE);
	NVIC_PRIORITY(7,1);

}
// Thêm khai báo này ở vùng biến toàn cục (phía trên hàm main)
volatile uint16_t adc_results[4];
DMA_Handle_t tft_dma,adc_dma;

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
    // ADC1 bắt buộc dùng DMA2 Stream 0, Channel 0
}
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
    adc_dma.dmay.priority = 2;                     // High priority

    adc_dma.dmay.per_addr = (uint32_t)&(ADC1->DR);  // Địa chỉ nguồn: ADC1 Data Register
    adc_dma.dmay.mem0_addr = (uint32_t)adc_results;  // Địa chỉ đích: Mảng RAM 3 phần tử
    adc_dma.dmay.number_of_data = 3;               // Chuyển đúng 3 kênh (Rank 1, 2, 3)

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
void ETH_INIT()
{
	eth.eth_mac=ETH_MAC;
	eth.eth_mmc=ETH_MMC;
	eth.eth_ieee=ETH_IEEE;
	eth.eth_dma=ETH_DMA;
	eth.eth_x.mode=ETH_RMII_MODE;
	eth.eth_x.phy_addr=1;
	eth.eth_x.mac_addr= 0x000100000002;
	eth.eth_x.ip_addr=0xC0A88932;//0xA9FEA9FE;
	eth.TX_BUFFER= (uint32_t*)(0x2001C000);
	eth.RX_BUFFER= (uint32_t*)(0x2001C800);
	eth.tx_state=IDLE;
	eth.rx_state=IDLE;
	config_tx_buffer(eth.TX_BUFFER,(uint32_t)tx_buffer,(uint32_t)eth.TX_BUFFER);
	config_rx_buffer(eth.RX_BUFFER, (uint32_t)rx_buffer, (uint32_t)eth.RX_BUFFER);
	ETH_Init(&eth);

	NVIC_ConFig(61,ENABLE);
	NVIC_PRIORITY(61,1);
}
void adc_init()
{
    // 1. Cấu hình các kênh ADC (Channel & Rank)
    adc1.adcx = ADC1;
    adc1.adcy.length_of_cvst = 2; // Quét 3 kênh (Length = N - 1 = 2)
    adc1.adcy.resolution = 0;     // 12-bit
    adc1.adcy.cycle = 7;          // Sample time

    adc1.adcy.channel = 8;
    adc1.adcy.rank = 1;
    ADC_Init(&adc1);

    adc1.adcy.channel = 3;
    adc1.adcy.rank = 2;
    ADC_Init(&adc1);

    adc1.adcy.channel = 4;
    adc1.adcy.rank = 3;
    ADC_Init(&adc1);

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

    x.gpiox = GPIOA;
    x.gpioy.pin = 3;
    GPIO_Init(&x);

    x.gpioy.pin = 4;
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
    for (volatile int i = 0; i < 1000; i++); // Chờ ADC ổn định
    ADC1->CR2 |= (1 << 0);  // ADON lần 2
    ADC1->CR2 |= (1 << 30); // SWSTART: Kích hoạt chuỗi chuyển đổi đầu tiên

    // Không cần cấu hình NVIC_ConFig(18, ENABLE) hay NVIC_PRIORITY cho ADC nữa!
}
void tftt_inti()
{

    gpio_init(); // Gọi hàm cấu hình chân đã sửa ở trên
    spi_init();  // Khởi tạo struct spi1 (đang trỏ tới SPI2)
    SPI2->CR2 |= (1 << 1);
	// 1. Nạp cấu hình vào Struct
	tft_dma_init();

	// 2. MẸO: Chặn không cho DMA tự chạy bằng cách ép số lượng = 0
	tft_dma.dmay.number_of_data = 0;

	// 3. Gọi Init để ốp toàn bộ cấu hình (Channel, MINC, DIR...) xuống thanh ghi
	// Đến dòng cuối cùng của hàm này, lệnh bật EN sẽ bị STM32 vô hiệu hóa vì NDTR = 0
	DMA_Init(&tft_dma);

	// 4. Khởi tạo màn hình
	SPI_Control(SPI2, ENABLE);
	SPI_Control(SPI1, ENABLE);
	tft_init(&spi1);
	tft_clear(&spi1, TFT_COLOR_BLACK);

	NVIC_ConFig(15,ENABLE);
	NVIC_PRIORITY(15,1);
}
uint16_t length;
__attribute__((aligned(4))) ETH_REI_DATA *rx_buff;
uint8_t type;
extern volatile uint8_t *udp_data;
extern volatile uint16_t payload_len;
volatile uint8_t frame_finished=0;
volatile TFT_Line_Element_t active_line;
volatile uint8_t touch=0,spi_locked = 0;


volatile uint8_t mode_menu=0;
FATFS fs;           // Vùng làm việc của FatFs cho thẻ nhớ
FIL myFile;         // Biến quản lý File
FRESULT res;        // Trạng thái trả về
volatile uint8_t sd_mounted = 1,read_active=0;
uint8_t direction=0;
volatile uint8_t adc_ready=0,channel_index = 0;

int main(void)
{
      CLK_init();
    adc_init();
    TFT_CS_HIGH();
    tftt_inti();
    TFT_RingBuffer_Init();

    // 1. Khởi tạo Thẻ nhớ SD (FatFS)
    sd_mounted = 0;
    if (f_mount(&fs, "", 1) == FR_OK) {
        sd_mounted = 1;
        SPI_Control(SPI1, DISABLE);
        spi.spiy.baurate = SPI_BAUD_4;
        SPI_Init(&spi);
        SPI_Control(SPI1, ENABLE);
    }

    // 2. Khởi tạo Cây Menu và Vẽ màn hình đầu tiên
    Menu_Init_Tree();
    Menu_Render(&spi1);

    while (1)
    {
        // A. Đọc trạng thái Joystick từ ADC
        MenuEvent_t joy_event = adc_handle((uint16_t*)adc_results);

        // B. Xử lý Logic Menu / Đọc file SD theo Title / Xử lý Ethernet Packet
        menu_handle(joy_event, &spi1);

        // --- 3. Luồng Vẽ DMA (Luôn chạy dưới nền) ---
        if (TFT_RingBuffer_Pop(&active_line))
        {
            spi_locked = 1;
            uint16_t start_y = active_line.line_id * 2;
            if(start_y >= 238) start_y = 238;
            tft_set_window(&spi1, 0, start_y, 319, start_y + 1);
            GPIO_WritePin(GPIOD, GPIO_PIN_2, SET);

            // ... (Giữ nguyên toàn bộ khối xử lý DMA cũ của bạn ở đây) ...
            TFT_DC_DATA();
            TFT_CS_LOW();
            DMA1->DMA_MEM[4].SCR &= ~(1 << 0);
            while (DMA1->DMA_MEM[4].SCR & (1 << 0));
            DMA1->HIFCR |= (0x3D << 0);
            DMA1->DMA_MEM[4].SM0AR = (uint32_t)active_line.color_data;
            DMA1->DMA_MEM[4].SNDTR = 1280;
            DMA1->DMA_MEM[4].SCR |= (1 << 0)|(1<<4);
            while (DMA1->DMA_MEM[4].SNDTR > 0);
            while (SPI2->SR & (1 << 7));

            volatile uint8_t temp;
            while (SPI2->SR & (1 << 0)) { temp = SPI2->DR; }
            temp = SPI2->SR;
            temp = SPI2->DR;
            (void)temp;

            TFT_CS_HIGH();
            spi_locked = 0;
        }

        if (frame_finished) {
            frame_finished = 0;
            //ADC1->CR2|=1<<30;
        }
    }
}
void ETH_IRQHandler ()
{
	ETH_IRQ_Handle(&eth);
}
void DMA1_Stream4_IRQHandler(void)
{
    // 1. Kiểm tra cờ TCIF4 (bit 5 trong HISR cho Stream 4)
    if (DMA1->HISR & (1 << 5))
    {
        // 2. Xóa cờ TCIF4 bằng HIFCR (viết 1 vào bit 5 để xóa)
        DMA1->HIFCR = (1 << 5);

        // KHÔNG GỌI TFT_CS_HIGH() VÀ spi_locked = 0 Ở ĐÂY NỮA
        frame_finished = 1;
    }

    // Xử lý các cờ lỗi cho Stream 4 (nếu cần)
    if (DMA1->HISR & (1 << 3)) { DMA1->HIFCR = (1 << 3); } // CTCIF4 (Transfer Error)
    if (DMA1->HISR & (1 << 2)) { DMA1->HIFCR = (1 << 2); } // CDMEIF4 (Direct Mode Error)
    if (DMA1->HISR & (1 << 0)) { DMA1->HIFCR = (1 << 0); } // CFEIF4 (FIFO Error)
}
void EXTI1_IRQHandler ()
{
	if(GPIO_ReadPin(GPIOD,GPIO_PIN_1)==1)
	adc_results[3]=EVENT_NONE;
	else adc_results[3]=EVENT_SELECT;
	GPIO_IRQ_Handle(1);
}
