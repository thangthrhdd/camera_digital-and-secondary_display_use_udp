/*
 * otg.h
 *
 *  Created on: Nov 18, 2025
 *      Author: ADMIN
 */

#ifndef INC_OTG_H_
#define INC_OTG_H_
#include "stm32f407.h"
#include "gpio.h"
typedef struct
{
		 uint8_t bmRequestType;
		 uint8_t bRequest;
		 uint16_t wValue;
		 uint16_t wIndex;
		 uint16_t wLength;
}USB_SETUP_PACKET;
typedef struct
{
	OTG_FS_GLOBAL *Global;
	OTG_FS_HOST*Host;
	OTG_FS_DEVICE *Device;
	OTG_FS_Power* PCGCCTL;
}OTG_FS_ConFig_t;
void OTG_FS_Core_Init(OTG_FS_ConFig_t*otg_fs);
void OTG_FS_Device_Init(OTG_FS_ConFig_t*otg_fs);
void CDC_SendData(OTG_FS_ConFig_t*otg_fs,uint8_t*data,uint16_t length);
#endif /* INC_OTG_H_ */
