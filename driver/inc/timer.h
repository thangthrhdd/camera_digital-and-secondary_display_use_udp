/*
 * timer.h
 *
 *  Created on: Nov 30, 2025
 *      Author: ADMIN
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_
#include"stm32f407.h"

typedef struct
{
	uint8_t mode;
	uint8_t ARR_control;
	uint8_t DIR_control;
	uint8_t one_pulse;
	uint16_t CNT;
	uint16_t ARR;
	uint16_t psc;

}TIM_ConFig_t;

typedef struct
{
	TIM_RegDef_t*timx;
	TIM_ConFig_t timy;
}TIM_Handle_t;
//define mode;
#define TIM_BASE_MODE 0

//ARR
#define TIM_ARR_EN 1
#define TIM_ARR_DI 0
//DIR
#define TIM_UPCNT 0
#define TIM_DOWNCNT 1
//one pulse
#define TIM_ONE_PULSE_EN 1
#define TIM_ONE_PULSE_DI 0

void TIM_CLOCK(TIM_RegDef_t*timx,uint8_t EnorDi);
void TIM_Init(TIM_Handle_t*tim_handle);
void TIM_Delay_us (TIM_RegDef_t*timx,uint32_t us,uint16_t arr);
void TIM_PWM (TIM_RegDef_t*timx,uint32_t period,uint8_t duty_cycle,uint8_t channel);
void TIM_IRQ_Handle(TIM_Handle_t*tim_handle);

#endif /* INC_TIMER_H_ */
