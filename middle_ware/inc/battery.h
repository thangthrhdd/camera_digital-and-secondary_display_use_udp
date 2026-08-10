/*
 * battary.h
 *
 *  Created on: Aug 8, 2026
 *      Author: ADMIN
 */

#ifndef INC_BATTERY_H_
#define INC_BATTERY_H_
#include "stm32f407.h"
#include "tftcolor.h"
#include "spi.h"
#include "stdint.h"
void Battery_Init(uint16_t adc_battery,uint16_t adc_vrefint, SPI_Handle_t* spi1);


#endif /* INC_BATTERY_H_ */
