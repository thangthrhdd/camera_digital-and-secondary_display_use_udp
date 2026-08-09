/*
 * ov7670.h
 *
 *  Created on: Aug 2, 2026
 *      Author: ADMIN
 */

#ifndef INC_OV7670_H_
#define INC_OV7670_H_

#include <stdint.h>
#include "i2c.h"

// Địa chỉ I2C 7-bit của OV7670 (0x42 >> 1 = 0x21)
#define OV7670_I2C_ADDR         0x21

// Các thanh ghi quan trọng của OV7670
#define OV7670_REG_GAIN             0x00
#define OV7670_REG_BLUE             0x01
#define OV7670_REG_RED              0x02
#define OV7670_REG_VREF             0x03
#define OV7670_REG_COM1             0x04
#define OV7670_REG_PID              0x0A
#define OV7670_REG_VER              0x0B
#define OV7670_REG_COM3             0x0C
#define OV7670_REG_COM4             0x0D
#define OV7670_REG_CLKRC            0x11
#define OV7670_REG_COM7             0x12
#define OV7670_REG_COM8             0x13
#define OV7670_REG_COM9             0x14
#define OV7670_REG_COM10            0x15
#define OV7670_REG_HSTART           0x17
#define OV7670_REG_HSTOP            0x18
#define OV7670_REG_VSTART           0x19
#define OV7670_REG_VSTOP            0x1A
#define OV7670_REG_MVFP             0x1E
#define OV7670_REG_AEW              0x24
#define OV7670_REG_AEB              0x25
#define OV7670_REG_VPT              0x26
#define OV7670_REG_HREF             0x32
#define OV7670_REG_COM11            0x3B
#define OV7670_REG_COM13            0x3D
#define OV7670_REG_COM14            0x3E
#define OV7670_REG_COM15            0x40
#define OV7670_REG_COM16            0x41
#define OV7670_REG_COM17            0x42

// Thanh ghi Scaling & Clock Division
#define OV7670_REG_SCALING_XSC      0x70
#define OV7670_REG_SCALING_YSC      0x71
#define OV7670_REG_SCALING_DCWCTR   0x72
#define OV7670_REG_SCALING_PCLK_DIV 0x73
#define OV7670_REG_SCALING_PCLK_DEL 0xA2

// Thanh ghi AEC/AGC
#define OV7670_REG_HAECC1           0x9F
#define OV7670_REG_HAECC2           0xA0
#define OV7670_REG_HAECC3           0xA1
#define OV7670_REG_HAECC4           0xA2
#define OV7670_REG_HAECC5           0xA3
#define OV7670_REG_HAECC6           0xA4
#define OV7670_REG_HAECC7           0xA5
#define OV7670_REG_BD50MAX          0xAB
#define OV7670_REG_BD60MAX          0xAC

// Cấu trúc chứa cặp (Thanh ghi, Giá trị)
typedef struct {
    uint8_t reg;
    uint8_t val;
} ov7670_reg_t;

// CÁC HÀM GIAO TIẾP VỚI CAMERA
uint8_t OV7670_Write_Reg(I2C_RegDef_t *i2cx, uint8_t reg, uint8_t data);
uint8_t OV7670_Read_Reg(I2C_RegDef_t *i2cx, uint8_t reg, uint8_t *data);
uint8_t OV7670_Init(I2C_RegDef_t *i2cx);

#endif /* INC_OV7670_H_ */
