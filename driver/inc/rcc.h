/*
 * rcc.h
 *
 *  Created on: Nov 17, 2025
 *      Author: ADMIN
 */

#ifndef INC_RCC_H_
#define INC_RCC_H_
#include "stm32f407.h"
typedef struct
{

}RCC_ConFig_t;

typedef struct
{
	RCC_RegDef_t *rccx;
	RCC_ConFig_t rccy;
}RCC_Handle_t;
void RCC_Init(RCC_Handle_t*rcc_handle);
#endif /* INC_RCC_H_ */
