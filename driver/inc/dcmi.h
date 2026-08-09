/*
 * dcmi.h
 *
 *  Created on: Aug 1, 2026
 *      Author: ADMIN
 */

#ifndef INC_DCMI_H_
#define INC_DCMI_H_
#include "stm32f407.h"
typedef struct
{
	uint8_t data_mode;
	uint8_t Frame_capture_rate;
	uint8_t Vertical_syn_polarity;
	uint8_t Horizontal_syn_polarity;
	uint8_t Pixel_clock_polarity;
	uint8_t hard_or_embed_mode;
	uint8_t JPEG_format;
	uint8_t Crop_feature;
	uint8_t captrue_mode;
}DCMI_ConFig_t;
typedef struct
{
	DCMI_RegDef_t *dcmix;
	DCMI_ConFig_t dcmiy;

}DCMI_Handle_t;
//data_mode
#define DCMI_8BIT 0
#define DMCI_10BIT 1
#define DMCI_12BIT 2
#define DCMI_14BIT 3
//define frame captrue rate
#define DCMI_All_frames 0
#define DCMI_alternate_frame 1
#define DCMI_frame_in_4_frame 2
//define vertical
#define DCMI_VSYN_HIGH 1
#define DCMI_VSYN_LOW 0
//hozrion
#define DCMI_HSYN_HIGH 1
#define DCMI_HSYN_LOW 0
//pclk
#define DCMI_PIXEL_RISING 1
#define DCMI_PIXEL_FALLING 0
//Embedded synchronization select
#define DMCI_HARD_MODE 0
#define DCMI_EMBEDDED_MODE 1
//JPEG forma
#define DCMI_Uncompressed 0
#define DMCI_JPEG 1
//Crop feature
#define DMCI_full_image 0
#define DMCI_CROP 1
//: Capture mode
#define DMCI_Continuous_grab 0
#define DMCI_SNAP_SHOT 1
void DCMI_Init(DCMI_Handle_t *dcmi_handle);
void DCMI_CAPTRUE(DCMI_Handle_t *dcmi_handle,uint8_t enordi);


#endif /* INC_DCMI_H_ */
