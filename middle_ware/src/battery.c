/*
 * battery.c
 *
 *  Created on: Aug 8, 2026
 *      Author: ADMIN
 */
#include "battery.h"
#include <stdio.h>
#include <stdio.h>
#include <stdint.h>


// Hàm hiển thị lên màn hình TFT
void Battery_Init(uint16_t adc_battery,uint16_t adc_vrefint, SPI_Handle_t* spi1)
{
	float vbat= 2*(1.21f*((float)adc_battery/(float)adc_vrefint));
	uint16_t percent=((float)(vbat-3.4f)/(float)(4.2f-2.4f))*100;
	char bat[25];
    sprintf(bat, "BAT:%3f%v",vbat);
    tft_write_string(spi1, 220, 5,bat, 0xFFFF, 0x0000);
    sprintf(bat, "BAT:%4d%%",percent);
    tft_write_string(spi1, 220, 20,bat, 0xFFFF, 0x0000);
}
