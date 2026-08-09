/*
 * adc.h
 *
 *  Created on: Dec 2, 2025
 *      Author: ADMIN
 */

#ifndef INC_ADC_H_
#define INC_ADC_H_
#include"stm32f407.h"
typedef struct
{
	uint8_t channel;
	uint8_t mode;
	uint8_t resolution;
	uint8_t rank;
	uint8_t cycle;
	uint8_t length_of_cvst;
}ADC_Config_t;

typedef struct
{
	ADC_COM_RegDef_t*adc_com;
	ADC_RegDef_t*adcx;
	ADC_Config_t adcy;
}ADC_Handle_t;
void ADC_Clock(ADC_RegDef_t*adcx,uint8_t EnorDi);
void ADC_Init(ADC_Handle_t*adc_handle);
void ADC_Read(ADC_RegDef_t*adcx,uint8_t length,uint16_t * buff);

#endif /* INC_ADC_H_ */
