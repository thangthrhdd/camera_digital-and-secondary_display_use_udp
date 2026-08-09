/*
 * battery.c
 *
 *  Created on: Aug 8, 2026
 *      Author: ADMIN
 */
#include "battery.h"
#include <stdio.h>

void Battery_Init(uint16_t adc_battery, SPI_Handle_t* spi1)
{
	static float adc_filtered = 0.0f;
	    static uint8_t last_percent = 255;
	    char bat[20];
	// 2. TRƯỜNG HỢP CHẠY PIN ĐỘC LẬP
	    if (adc_filtered == 0.0f) {
	        adc_filtered = (float)adc_battery;
	    }

	    // Lọc dập nhiễu ADC
	    adc_filtered = (adc_filtered * 0.92f) + ((float)adc_battery * 0.08f);
	    uint16_t adc_smooth = (uint16_t)adc_filtered;

	    // Tính % Pin theo mốc chuẩn 2530 -> 3130
	    uint8_t percent;
	    if (adc_smooth <= 2530) {
	        percent = 0;
	    }
	    else if (adc_smooth >= 3130) {
	        percent = 100;
	    }
	    else {
	        percent = (uint8_t)(((adc_smooth - 2530) * 100) / 600);
	    }

	    // Chỉ vẽ lại khi phần trăm thay đổi
	    if (percent != last_percent)
	    {
	        last_percent = percent;
	        sprintf(bat, "BAT:%3d%%", percent);
	        tft_write_string(spi1, 220, 5, bat, 0xFFFF, 0x0000); // Chữ TRẮNG
	    }
   // char bat[20];
      sprintf(bat, "BAT:%4d%%", adc_battery);
      tft_write_string(spi1, 220, 15, bat, 0xFFFF, 0x0000);
}

