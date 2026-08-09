/*
 * gpio.h
 *
 *  Created on: Nov 3, 2025
 *      Author: ADMIN
 */

#ifndef INC_GPIO_H_
#define INC_GPIO_H_
#include"stm32f407.h"
typedef struct
{
  uint8_t mode;
  uint8_t optype;
  uint8_t ospeed;
  uint8_t pupd;
  uint8_t alt_num;
  uint8_t pin;
}GPIO_ConFig_t;

typedef struct
{
	GPIO_RegDef_t *gpiox;
	GPIO_ConFig_t gpioy;
}GPIO_Handle_t;
void GPIO_CLK(GPIO_RegDef_t*gpiox,uint8_t EnorDi);
void GPIO_Init(GPIO_Handle_t*gpio_handle);
void GPIO_WritePin(GPIO_RegDef_t*gpiox,uint8_t pin,uint8_t sta);
uint8_t GPIO_ReadPin(GPIO_RegDef_t*gpiox,uint8_t pin);
void GPIO_TogglePin(GPIO_RegDef_t*gpiox,uint8_t pin);

void NVIC_ConFig(uint8_t IRQ_number,uint8_t EnorDi);
void NVIC_PRIORITY(uint8_t IRQ_number,uint8_t priority);
void GPIO_IRQ_Handle(uint8_t pin);
#define GPIO_To_Code(X) ((X==GPIOA)?0:(X==GPIOB)?1: (X==GPIOC)?2: (X==GPIOD)?3:(X==GPIOE)?4:(X==GPIOF)?5:(X==GPIOG)?6:(X==GPIOH)?7:(X==GPIOI)?8:0)
//moder
#define GPIO_INPUT_MODE 0
#define GPIO_OUTPUT 1
#define GPIO_ALTFUNC 2
#define GPIO_ANALOG_MODE 3

#define EXTI_RT 4
#define EXTI_FT 5
#define EXTI_RTFT 6
//optype
#define GPIO_OUTPUT_PP 0
#define GPIO_OUTPUT_OPDR 1
//speed
#define GPIO_LOW_SPEED 0
#define GPIO_MEDIUM_SPEED 1
#define GPIO_HIGH_SPEED 2
#define GPIO_VRY_HIGH_SPEED 3
//pupd
#define GPIO_NO_PUPD 0
#define GPIO_PU 1
#define GPIO_PD 2
//pin
#define GPIO_PIN_0 0
#define GPIO_PIN_1 1
#define GPIO_PIN_2 2
#define GPIO_PIN_3 3
#define GPIO_PIN_4 4
#define GPIO_PIN_5 5
#define GPIO_PIN_6 6
#define GPIO_PIN_7 7
#define GPIO_PIN_8 8
#define GPIO_PIN_9 9
#define GPIO_PIN_10 10
#define GPIO_PIN_11 11
#define GPIO_PIN_12 12
#define GPIO_PIN_13 13
#define GPIO_PIN_14 14
#define GPIO_PIN_15 15
//define af pin
#define GPIO_PIN_AF0 0
#define GPIO_PIN_AF1 1
#define GPIO_PIN_AF2 2
#define GPIO_PIN_AF3 3
#define GPIO_PIN_AF4 4
#define GPIO_PIN_AF5 5
#define GPIO_PIN_AF6 6
#define GPIO_PIN_AF7 7
#define GPIO_PIN_AF8 8
#define GPIO_PIN_AF9 9
#define GPIO_PIN_AF10 10
#define GPIO_PIN_AF11 11
#define GPIO_PIN_AF12 12
#define GPIO_PIN_AF13 13
#define GPIO_PIN_AF14 14
#define GPIO_PIN_AF15 15
#endif /* INC_GPIO_H_ */
