/*
 * stm32f407.h
 *
 *  Created on: Nov 3, 2025
 *      Author: ADMIN
 */

#ifndef INC_STM32F407_H_
#define INC_STM32F407_H_

#include<stdint.h>
#include "stdlib.h"
#define __vo volatile
//NVIC
#define NVIC_ISER_BASEADDR 0xE000E100U
#define NVIC_ICER_BASEADDR 0XE000E180U
#define NVIC_IPR_BASEADDR  0xE000E400U
#define FPU_CPACR *((__vo uint32_t*)(0xE000ED88))
#define FPU_BASEADDR 0xE000EF34U
//system ADDr
#define RAM_ADDR 0x20000000U
#define FLASH_ADDR 0x08000000U
#define ROM_ADDR 0x1FFF0000U
//bus addr
#define AHB3_ADDR 0xA0000000U
#define AHB2_ADDR 0x50000000U
#define AHB1_ADDR 0x40020000U
#define APB2_ADDR 0x40010000U
#define APB1_ADDR 0x40000000U
 //peripheral addr
#define RCC_BASEADDR (AHB1_ADDR+0x3800)
#define GPIOA_BASEADDR (AHB1_ADDR)
#define GPIOB_BASEADDR (AHB1_ADDR+0x0400)
#define GPIOC_BASEADDR (AHB1_ADDR+0x0800)
#define GPIOD_BASEADDR (AHB1_ADDR+0x0C00)
#define GPIOE_BASEADDR (AHB1_ADDR+0x1000)
#define GPIOF_BASEADDR (AHB1_ADDR+0x1400)
#define GPIOG_BASEADDR (AHB1_ADDR+0x1800)
#define GPIOH_BASEADDR (AHB1_ADDR+0x1C00)
#define GPIOI_BASEADDR (AHB1_ADDR+0x2000)
#define DMA1_BASEADDR (AHB1_ADDR+0x6000)
#define DMA2_BASEADDR (AHB1_ADDR+0x6400)

#define SYSCFG_BASEADDR (APB2_ADDR+0x3800)
#define EXTI_BASEADDR (APB2_ADDR+0x3C00)

#define SPI1_BASEADDR (APB2_ADDR+0x3000)
#define SPI2_BASEADDR (APB1_ADDR+0x3800)
#define SPI3_BASEADDR (APB1_ADDR+0x3C00)

#define USART1_BASEADDR (APB2_ADDR+0x1000)
#define USART6_BASEADDR (APB2_ADDR+0x1400)
#define UART2_BASEADDR (APB1_ADDR +0x4400)
#define UART3_BASEADDR (APB1_ADDR +0x4800)
#define UART4_BASEADDR (APB1_ADDR +0x4C00)
#define UART5_BASEADDR (APB1_ADDR +0x5000)

#define I2C1_BASEADDR (APB1_ADDR +0x5400)
#define I2C2_BASEADDR (APB1_ADDR +0x5800)
#define I2C3_BASEADDR (APB1_ADDR +0x5C00)


#define TIM1_BASEADDR (APB2_ADDR)
#define TIM2_BASEADDR (APB1_ADDR)
#define TIM8_BASEADDR (APB2_ADDR+0x400)

#define OTG_FS_BASEADDR 0x50000000U

#define ADC_BASEADDR 0x40012000U

#define ETHERNET_MAC_BASEADDR 0x40028000U

#define BXCAN_BASEADDR (APB1_ADDR +0x6400)
#define DCMI_BASEADDR (AHB2_ADDR+0x50000)

typedef struct
{
	__vo uint32_t CR;
	__vo uint32_t PLLCFGR;
	__vo uint32_t CFGR;
	__vo uint32_t CIR;
	__vo uint32_t AHB1RSTR;
	__vo uint32_t AHB2RSTR;
	__vo uint32_t AHB3RSTR;
	__vo uint32_t Reserved0;

	__vo uint32_t APB1RSTR;
	__vo uint32_t APB2RSTR;
	__vo uint32_t Reserved1[2];

	__vo uint32_t AHB1ENR;
	__vo uint32_t AHB2ENR;
	__vo uint32_t AHB3ENR;
	__vo uint32_t Reserved2;

	__vo uint32_t APB1ENR;
	__vo uint32_t APB2ENR;
	__vo uint32_t Reserved3[2];

	__vo uint32_t AHB1LPENR;
	__vo uint32_t AHB2LPENR;
	__vo uint32_t AHB3LPENR;
	__vo uint32_t Reserved4;

	__vo uint32_t APB1LPENR;
	__vo uint32_t APB2LPENR;
	__vo uint32_t Reserved5[2];

	__vo uint32_t BDCR;
	__vo uint32_t CSR;
	__vo uint32_t Reserved6[2];

	__vo uint32_t SSCGR;
	__vo uint32_t PLLI2SCFGR;

}RCC_RegDef_t;
#define RCC ((RCC_RegDef_t*)RCC_BASEADDR)

//clock
#define GPIOA_PCLK_EN() (RCC->AHB1ENR|=1<<0)
#define GPIOB_PCLK_EN() (RCC->AHB1ENR|=1<<1)
#define GPIOC_PCLK_EN() (RCC->AHB1ENR|=1<<2)
#define GPIOD_PCLK_EN() (RCC->AHB1ENR|=1<<3)
#define GPIOE_PCLK_EN() (RCC->AHB1ENR|=1<<4)
#define GPIOF_PCLK_EN() (RCC->AHB1ENR|=1<<5)
#define GPIOG_PCLK_EN() (RCC->AHB1ENR|=1<<6)
#define GPIOH_PCLK_EN() (RCC->AHB1ENR|=1<<7)
#define GPIOI_PCLK_EN() (RCC->AHB1ENR|=1<<8)
#define DMA1_PCLK_EN() (RCC->AHB1ENR|=1<<21)
#define DMA2_PCLK_EN() (RCC->AHB1ENR|=1<<22)
#define SYSCFG_PCLK_EN() (RCC->APB2ENR|=1<<14)

#define SPI1_PCLK_EN()  (RCC->APB2ENR|=1<<12)
#define SPI2_PCLK_EN()  (RCC->APB1ENR|=1<<14)
#define SPI3_PCLK_EN()  (RCC->APB1ENR|=1<<15)

#define USART1_PCLK_EN() (RCC->APB2ENR|=1<<4)
#define USART6_PCLK_EN() (RCC->APB2ENR|=1<<5)
#define UART2_PCLK_EN() (RCC->APB1ENR|=1<<17)
#define UART3_PCLK_EN() (RCC->APB1ENR|=1<<18)
#define UART4_PCLK_EN() (RCC->APB1ENR|=1<<19)
#define UART5_PCLK_EN() (RCC->APB1ENR|=1<<20)

#define I2C1_PCLK_EN() (RCC->APB1ENR|=1<<21)
#define I2C2_PCLK_EN() (RCC->APB1ENR|=1<<22)
#define I2C3_PCLK_EN() (RCC->APB1ENR|=1<<23)

#define OTG_PCLK_EN() (RCC->AHB2ENR|=1<<7)
#define DCMI_PCLK_EN()(RCC->AHB2ENR|=1<<0)

#define TIM1_PCLK_EN() (RCC->APB2ENR|=1<<0)
#define TIM2_PCLK_EN() (RCC->APB1ENR|=(1<<0))
#define TIM8_PCLK_EN() (RCC->APB2ENR|=1<<1)

#define ADC1_PCLK_EN() (RCC->APB2ENR|=(1<<8))
#define ADC2_PCLK_EN() (RCC->APB2ENR|=(1<<9))
#define ADC3_PCLK_EN() (RCC->APB2ENR|=(1<<10))

#define ETH_MAC_PCLK_EN() (RCC->AHB1ENR|=(1<<25))
#define ETH_TX_PCLK_EN() (RCC->AHB1ENR|=(1<<26))
#define ETH_RX_PCLK_EN() (RCC->AHB1ENR|=(1<<27))
#define ETH_PTP_PCLK_EN() (RCC->AHB1ENR|=(1<<28))

#define CAN1_PCLK_EN() (RCC->APB1ENR|=(1<<25))
#define CAN2_PCLK_EN() (RCC->APB1ENR|=(1<<26))
//clock dis
#define GPIOA_PCLK_DI() (RCC->AHB1ENR&=~(1<<0))
#define GPIOB_PCLK_DI() (RCC->AHB1ENR&=~(1<<1))
#define GPIOC_PCLK_DI() (RCC->AHB1ENR&=~(1<<2))
#define GPIOD_PCLK_DI() (RCC->AHB1ENR&=~(1<<3))
#define GPIOE_PCLK_DI() (RCC->AHB1ENR&=~(1<<4))
#define GPIOF_PCLK_DI() (RCC->AHB1ENR&=~(1<<5))
#define GPIOG_PCLK_DI() (RCC->AHB1ENR&=~(1<<6))
#define GPIOH_PCLK_DI() (RCC->AHB1ENR&=~(1<<7))
#define GPIOI_PCLK_DI() (RCC->AHB1ENR&=~(1<<8))

#define DMA1_PCLK_DI() (RCC->AHB1ENR&=~(1<<21))
#define DMA2_PCLK_DI() (RCC->AHB1ENR&=~(1<<22))
#define SYSCFG_PCLK_DI() (RCC->APB2ENR&=~(1<<14))

#define SPI1_PCLK_DI()  (RCC->APB2ENR&=~(1<<12))
#define SPI2_PCLK_DI()  (RCC->APB1ENR&=~(1<<14))
#define SPI3_PCLK_DI()  (RCC->APB1ENR&=~(1<<15))

#define USART1_PCLK_DI() (RCC->APB2ENR&=~(1<<4))
#define USART6_PCLK_DI() (RCC->APB2ENR&=~(1<<5))
#define UART2_PCLK_DI() (RCC->APB1ENR&=~(1<<17))
#define UART3_PCLK_DI() (RCC->APB1ENR&=~(1<<18))
#define UART4_PCLK_DI() (RCC->APB1ENR&=~(1<<19))
#define UART5_PCLK_DI() (RCC->APB1ENR&=~(1<<20))

#define I2C1_PCLK_DI() (RCC->APB1ENR&=~(1<<21))
#define I2C2_PCLK_DI() (RCC->APB1ENR&=~(1<<22))
#define I2C3_PCLK_DI() (RCC->APB1ENR&=~(1<<23))

#define TIM1_PCLK_DI() (RCC->APB2ENR&=~(1<<0))
#define TIM2_PCLK_DI() (RCC->APB1ENR&=~(1<<0))
#define TIM8_PCLK_DI() (RCC->APB2ENR&=~(1<<1))

#define ADC1_PCLK_DI() (RCC->APB2ENR&=~(1<<8))
#define ADC2_PCLK_DI() (RCC->APB2ENR&=~(1<<9))
#define ADC3_PCLK_DI() (RCC->APB2ENR&=~(1<<10))

#define OTG_PCLK_DI() (RCC->AHB2ENR&=~(1<<7))
#define DCMI_PCLK_DI()(RCC->AHB2ENR&=~(1<<0))

#define TIM1_PCLK_DI() (RCC->APB2ENR&=~(1<<0))
#define TIM2_PCLK_DI() (RCC->APB1ENR&=~(1<<0))
#define TIM8_PCLK_DI() (RCC->APB2ENR&=~(1<<1))


#define ETH_MAC_PCLK_DI() (RCC->AHB1ENR&=~(1<<25))
#define ETH_TX_PCLK_DI() (RCC->AHB1ENR&=~(1<<26))
#define ETH_RX_PCLK_DI() (RCC->AHB1ENR&=~(1<<27))
#define ETH_PTP_PCLK_DI() (RCC->AHB1ENR&=~(1<<28))

#define CAN1_PCLK_DI() (RCC->APB1ENR&=~(1<<25))
#define CAN2_PCLK_DI() (RCC->APB1ENR&=~(1<<26))

typedef struct
{
	__vo uint32_t MODER;
	__vo uint32_t OTYPER;
	__vo uint32_t OSPEEDR;
	__vo uint32_t PUPDR;
	__vo uint32_t IDR;
	__vo uint32_t ODR;
	__vo uint32_t BSRR;
	__vo uint32_t LCKR;
	__vo uint32_t AFRL;
	__vo uint32_t AFRH;

}GPIO_RegDef_t;

#define GPIOA ((GPIO_RegDef_t*)GPIOA_BASEADDR)
#define GPIOB ((GPIO_RegDef_t*)GPIOB_BASEADDR)
#define GPIOC ((GPIO_RegDef_t*)GPIOC_BASEADDR)
#define GPIOD ((GPIO_RegDef_t*)GPIOD_BASEADDR)
#define GPIOE ((GPIO_RegDef_t*)GPIOE_BASEADDR)
#define GPIOF ((GPIO_RegDef_t*)GPIOF_BASEADDR)
#define GPIOG ((GPIO_RegDef_t*)GPIOG_BASEADDR)
#define GPIOH ((GPIO_RegDef_t*)GPIOH_BASEADDR)
#define GPIOI ((GPIO_RegDef_t*)GPIOI_BASEADDR)
typedef struct
{
	__vo uint32_t MEMRMP;
	__vo uint32_t PMC;
	__vo uint32_t EXTICR[4];
	__vo uint32_t CMPCR;
}SYSCFG_RegDef_t;
#define SYSCFG ((SYSCFG_RegDef_t*)SYSCFG_BASEADDR)

typedef struct
{
	__vo uint32_t IMR;
	__vo uint32_t EMR;
	__vo uint32_t RTSR;
	__vo uint32_t FTSR;
	__vo uint32_t SWIER;
	__vo uint32_t PR;
}EXTI_RegDef_t;
#define EXTI ((EXTI_RegDef_t*)EXTI_BASEADDR)

typedef struct
{
	__vo uint32_t CR1;
	__vo uint32_t CR2;
	__vo uint32_t SR;
	__vo uint32_t DR;
	__vo uint32_t CRCPR;
	__vo uint32_t RXCRCR;
	__vo uint32_t TXCRCR;
	__vo uint32_t I2SCFGR;
	__vo uint32_t I2SPR;
}SPI_RegDef_t;
#define SPI1 ((SPI_RegDef_t*)SPI1_BASEADDR)
#define SPI2 ((SPI_RegDef_t*)SPI2_BASEADDR)
#define SPI3 ((SPI_RegDef_t*)SPI3_BASEADDR)

typedef struct
{
	__vo uint32_t SR;
	__vo uint32_t DR;
	__vo uint32_t BRR;
	__vo uint32_t CR1;
	__vo uint32_t CR2;
	__vo uint32_t CR3;
	__vo uint32_t GTPR;

}UART_RegDef_t;
#define USART1 ((UART_RegDef_t*)USART1_BASEADDR)
#define USART6 ((UART_RegDef_t*)USART6_BASEADDR)
#define UART2 ((UART_RegDef_t*)UART2_BASEADDR)
#define UART3 ((UART_RegDef_t*)UART3_BASEADDR)
#define UART4 ((UART_RegDef_t*)UART4_BASEADDR)
#define UART5 ((UART_RegDef_t*)UART5_BASEADDR)

typedef struct
{
	__vo uint32_t CR1;
	__vo uint32_t CR2;
	__vo uint32_t OAR1;
	__vo uint32_t OAR2;
	__vo uint32_t DR;
	__vo uint32_t SR1;
	__vo uint32_t SR2;
	__vo uint32_t CCR;
	__vo uint32_t TRISE;
	__vo uint32_t FLTR;

}I2C_RegDef_t;
#define I2C1 ((I2C_RegDef_t*)I2C1_BASEADDR)
#define I2C2 ((I2C_RegDef_t*)I2C2_BASEADDR)
#define I2C3 ((I2C_RegDef_t*)I2C3_BASEADDR)


typedef struct
{
	__vo uint32_t GOTGCTL;//0x00
	__vo uint32_t GOTGINT;//0x04
	__vo uint32_t GAHBCFG;//0x08
	__vo uint32_t GUSBCFG;//0x0c
	__vo uint32_t GRSTCTL;//0x10
	__vo uint32_t GINTSTS;//0x14
	__vo uint32_t GINTMSK;//0x18
	__vo uint32_t GRXSTSP;//0x1c
	__vo uint32_t GRXSTSPR;//0x20
	__vo uint32_t GRXFSIZ;//0x24
	__vo uint32_t DIEPTXF0;//0x28
	__vo uint32_t HNPTXSTS;//0x2c
	__vo uint32_t Reversed1[2];
	__vo uint32_t GCCFG;//0x38
	__vo uint32_t CID;//0x3c
	__vo uint32_t Reversed2[48];
	__vo uint32_t HPTXFSIZ;//0x100
	__vo uint32_t DIEPTXF[3];
}OTG_FS_GLOBAL;
#define OTG_GLOBAL ((OTG_FS_GLOBAL*)OTG_FS_BASEADDR)
typedef struct
{
	__vo uint32_t HCCHAR;//0x500
	__vo uint32_t Reversed0;//0x504
	__vo uint32_t HCINT;//0x508
	__vo uint32_t HCINTMSK;//0x50c
	__vo uint32_t HCTSIZ;//0x510
	__vo uint32_t Reversed1[7];
}REG_HOST;
typedef struct
{
//	__vo uint32_t Reversed0[255];
	__vo uint32_t HCFG;//0x400
	__vo uint32_t HFIR;//0x404
	__vo uint32_t HFNUM;//0x408
	__vo uint32_t HPTXSTS;//0x410
	__vo uint32_t HAINT;//0x414
	__vo uint32_t HAINTMSK;//0x418
	__vo uint32_t Reversed1[9];
	__vo uint32_t HPRT;//0x440
	__vo uint32_t Reversed2[47];
	REG_HOST Host[8];
}OTG_FS_HOST;
#define OTG_HOST ((OTG_FS_HOST*)(OTG_FS_BASEADDR+0x400U))

typedef struct
{
	__vo uint32_t DIEPCTL;//0x900+0x20*x
	__vo uint32_t Reversed0;
	__vo uint32_t DIEPINT;//0x908+0x20*x
	__vo uint32_t Reversed5;
	__vo uint32_t DIEPTSIZ;//0x910+0x20*x
	__vo uint32_t Reversed1;
	__vo uint32_t DTXFSTS;//0x918+0x20*x
	__vo uint32_t Reversed2[1];
}REG_DIEP;

typedef struct
{
	__vo uint32_t DOEPCTL;//0xB00+0x20*x
	__vo uint32_t Reversed0;
	__vo uint32_t DOEPINT;//0xB08+0x20*x
	__vo uint32_t Reversed1;
	__vo uint32_t DOEPTSIZ;//0xB10+0x20*x
	__vo uint32_t Reversed2[3];
}REG_DOEP;
typedef struct
{
//	__vo uint32_t Reversed0[511];
	__vo uint32_t DCFG;//0x800
	__vo uint32_t DCTL;//0x804
	__vo uint32_t DSTS;//0x808
	__vo uint32_t Reversed1;//0x80c
	__vo uint32_t DIEPMSK;//0x810
	__vo uint32_t DOEPMSK;//0x814
	__vo uint32_t DAINT;//0x818
	__vo uint32_t DAINTMSK;//0x81c
	__vo uint32_t Reversed2[2];
	__vo uint32_t DVBUSDIS;//0x828
	__vo uint32_t DVBUSPULSE;//0x82c
	__vo uint32_t Reversed3;
	__vo uint32_t DIEPEMPMSK;//0x834

	__vo uint32_t Reversed4[50];//0x8fc
	REG_DIEP DIEP[4];//0x9CC
	__vo uint32_t Reversed6[96];
	REG_DOEP DOEP[4];//0xb00
}OTG_FS_DEVICE;
#define OTG_DEVICE ((OTG_FS_DEVICE*)(OTG_FS_BASEADDR+0x800U))
typedef struct
{
	__vo uint32_t PCGCCTL;
}OTG_FS_Power;
#define OTG_POWER ((OTG_FS_Power*)(OTG_FS_BASEADDR+0xE00U))

typedef struct
{
	__vo uint32_t CR1;//0x00
	__vo uint32_t CR2;//0x04
	__vo uint32_t SMCR;//0x08
	__vo uint32_t DIER;//0x0c
	__vo uint32_t SR;//0x10
	__vo uint32_t EGR;//0x14
	__vo uint32_t CCMR[2];//0x18
	__vo uint32_t CCER;//0x20
	__vo uint32_t CNT;//0x24
	__vo uint32_t PSC;//0x28
	__vo uint32_t ARR;//0x2c
	__vo uint32_t RCR;//0x30
	__vo uint32_t CCR[4];//0x34
	__vo uint32_t BDTR;//0x44
	__vo uint32_t DCR;//0x48
	__vo uint32_t DMAR;//0x4c
	__vo uint32_t OR;//0x50

}TIM_RegDef_t;
#define TIM1 ((TIM_RegDef_t*)TIM1_BASEADDR)
#define TIM2 ((TIM_RegDef_t*)TIM2_BASEADDR)
#define TIM8 ((TIM_RegDef_t*)TIM8_BASEADDR)


typedef struct
{
	__vo uint32_t SR;//0x50
	__vo uint32_t CR1;//0x50
	__vo uint32_t CR2;//0x50
	__vo uint32_t SMPR[2];
	__vo uint32_t JOFR[4];//0x50
	__vo uint32_t HTR;//0x50
	__vo uint32_t LTR;//0x50
	__vo uint32_t SQR[3];//0x50
	__vo uint32_t JSQR;//0x50
	__vo uint32_t JDR[4];//0x50
	__vo uint32_t DR;//0x50
}ADC_RegDef_t;
#define ADC1 ((ADC_RegDef_t*)ADC_BASEADDR)
#define ADC2 ((ADC_RegDef_t*)(ADC_BASEADDR+0x100))
#define ADC3 ((ADC_RegDef_t*)(ADC_BASEADDR+0x200))
typedef struct
{
	__vo uint32_t CSR;
	__vo uint32_t CCR;
	__vo uint32_t CDR;

}ADC_COM_RegDef_t;
#define ADC_COM ((ADC_COM_RegDef_t*)(ADC_BASEADDR+0x300))


typedef struct
{
	__vo uint32_t MACCR; //00
	__vo uint32_t MACFFR;//0x4
	__vo uint32_t MACHTHR;//0x8
	__vo uint32_t MACHTLR;//0xc
	__vo uint32_t MACMIIAR;//0x10
	__vo uint32_t MACMIIDR;//0x14
	__vo uint32_t MACFCR;//0x18
	__vo uint32_t MACVLANTR;//0x1c
	__vo uint32_t Reversed[2];

	__vo uint32_t MACRWUFFR;//0x28
	__vo uint32_t MACPMTCSR;//0x2c
	__vo uint32_t Reversed1;

	__vo uint32_t MACDBGR;//0x34
	__vo uint32_t MACSR;//0x38
	__vo uint32_t MACIMR;//0x3c
	__vo uint32_t MACA0HR;//0x40
	__vo uint32_t MACA0LR;//0x44
	__vo uint32_t MACA1HR;//0x48
	__vo uint32_t MACA1LR;//0x4c
	__vo uint32_t MACA2HR;//0x50
	__vo uint32_t MACA2LR;//0x54
	__vo uint32_t MACA3HR;//0x58
	__vo uint32_t MACA3LR;//0x5C
}ETH_MAC_RegDef_t;
#define ETH_MAC ((ETH_MAC_RegDef_t*)ETHERNET_MAC_BASEADDR)
typedef struct
{
	__vo uint32_t MMCCR;//0x100
	__vo uint32_t MMCRIR;//0x104
	__vo uint32_t MMCTIR;//0x108
	__vo uint32_t MMCRIMR;//0x10c
	__vo uint32_t MMCTIMR;//0x110
	__vo uint32_t Reversed[14];

	__vo uint32_t MMCTGFSCCR; //0x14c
	__vo uint32_t MMCTGFMSCCR;//0x150;
	__vo uint32_t Reversed1[5];

	__vo uint32_t MMCTGFCR;//0x168
	__vo uint32_t Reversed2[10];

	__vo uint32_t MMCRFCECR;//0x194
	__vo uint32_t MMCRFAECR;//0x198
	__vo uint32_t Reversed3[10];

	__vo uint32_t MMCRGUFCR;//0x1c4
}ETH_MMC_RegDef_t;
#define ETH_MMC ((ETH_MMC_RegDef_t*)(ETHERNET_MAC_BASEADDR+0x100))
typedef struct
{
	__vo uint32_t PTPTSCR;//0x700
	__vo uint32_t PTPSSIR;//0x704
	__vo uint32_t PTPTSHR;//0x708
	__vo uint32_t PTPTSLR;//0x70C
	__vo uint32_t PTPTSHUR;//0x710
	__vo uint32_t PTPTSLUR;//0x714
	__vo uint32_t PTPTSAR;//0x718
	__vo uint32_t PTPTTHR;//0x71C
	__vo uint32_t PTPTTLR;//0x720
	__vo uint32_t Reversed;
	__vo uint32_t PTPTSSR;//0x728
	__vo uint32_t PTPPPSCR;//0x72C

}ETH_IEEE_1588;
#define ETH_IEEE ((ETH_IEEE_1588*)(ETHERNET_MAC_BASEADDR+0x700))
typedef struct
{
	__vo uint32_t DMABMR;//0x1000
	__vo uint32_t DMATPDR;//0x1004
	__vo uint32_t DMARPDR;//0x1008
	__vo uint32_t DMARDLAR;//0x100C
	__vo uint32_t DMATDLAR;//0x1010
	__vo uint32_t DMASR;//0x1014
	__vo uint32_t DMAOMR;//0x1018
	__vo uint32_t DMAIER;//0x101C
	__vo uint32_t DMAMFBOCR;//0x1020
	__vo uint32_t DMARSWTR;//0x1024
	__vo uint32_t DMACHTDR;//0x1048
	__vo uint32_t DMACHRDR;//0x104C
	__vo uint32_t DMACHTBAR;//0x1050
	__vo uint32_t DMACHRBAR ;//0x1054

}ETH_DMA_RegDef_t;
#define ETH_DMA ((ETH_DMA_RegDef_t*)(ETHERNET_MAC_BASEADDR+0x1000))
typedef struct
{
	__vo uint32_t MCR;//0x00
	__vo uint32_t MSR;//0x04
	__vo uint32_t TSR;//0x08
	__vo uint32_t RFR[2];//0x0c,0x010
	__vo uint32_t IER;//0x14
	__vo uint32_t ESR;//0x18
	__vo uint32_t BTR;//0x1c
}CAN_Control_Reg_t;
typedef struct
{
	__vo uint32_t TIR;
	__vo uint32_t TDTR;
	__vo uint32_t TDLR;
	__vo uint32_t TDHR;
}CAN_TX_Mailbox_t;
typedef struct
{
	__vo uint32_t RIR;
	__vo uint32_t RDTR;
	__vo uint32_t RLR;
	__vo uint32_t RHR;
}CAN_RX_Mailbox_t;
typedef struct
{
	CAN_TX_Mailbox_t TX[3];
	CAN_RX_Mailbox_t RX[2];
}CAN_MailBox_Reg_t;
typedef struct
{
	__vo uint32_t FMR;
	__vo uint32_t FM1R;
	__vo uint32_t reversed;
	__vo uint32_t FS1R;
	__vo uint32_t reversed1;
	__vo uint32_t FFA1R;
	__vo uint32_t reversed2;
	__vo uint32_t FA1R;
	__vo uint32_t reversed3[8];
	__vo uint32_t FR[28][2];

}CAN_Filter_Reg_t;
typedef struct
{
	CAN_Control_Reg_t CAN_Control;
	__vo uint32_t reversed[88];
	CAN_MailBox_Reg_t CAN_Mailbox;
	__vo uint32_t reversed1[12];
	CAN_Filter_Reg_t CAN_Filter;
}CAN_RegDef_t;
#define CAN1 ((CAN_RegDef_t*)BXCAN_BASEADDR)
#define CAN2 ((CAN_RegDef_t*)(BXCAN_BASEADDR+0x400))



typedef struct
{
	__vo uint32_t FPCCR;
	__vo uint32_t FPCAR;
	__vo uint32_t FPDSCR;

}FPU_RegDef_t;
#define FPU ((FPU_RegDef_t*)FPU_BASEADDR)
typedef struct
{
	__vo uint32_t SCR;
	__vo uint32_t SNDTR;
	__vo uint32_t SPAR;
	__vo uint32_t SM0AR;
	__vo uint32_t SM1AR;
	__vo uint32_t SFCR;
}DMA_MEM_t;
typedef struct
{
	__vo uint32_t LISR;
	__vo uint32_t HISR;
	__vo uint32_t LIFCR;
	__vo uint32_t HIFCR;
	DMA_MEM_t DMA_MEM[8];
}DMA_RegDef_t;
#define DMA1 ((DMA_RegDef_t*)DMA1_BASEADDR)
#define DMA2 ((DMA_RegDef_t*)DMA2_BASEADDR)

typedef struct
{
	__vo uint32_t CR;
	__vo uint32_t SR;
	__vo uint32_t RIS;
	__vo uint32_t IER;
	__vo uint32_t MIS;
	__vo uint32_t ICR;
	__vo uint32_t ESCR;
	__vo uint32_t ESUR;
	__vo uint32_t CWSTRT;
	__vo uint32_t CWSIZE;
	__vo uint32_t DR;

}DCMI_RegDef_t;
#define DCMI ((DCMI_RegDef_t*) DCMI_BASEADDR )
#define SET 1
#define RESET 0
#define FLAG_SET 1
#define FLAG_RESET 0
#define ENABLE 1
#define DISABLE 0
#endif /* INC_STM32F407_H_ */
