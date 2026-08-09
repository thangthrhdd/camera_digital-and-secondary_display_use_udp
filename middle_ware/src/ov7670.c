#include "ov7670.h"

/*static const ov7670_reg_t ov7670_qvga_rgb565[]= {
		   {OV7670_REG_COM7,  0x80}, // Reset
		    {0xFF, 0xFF},             // Delay

		    {OV7670_REG_CLKRC, 0x01}, // PCLK = XCLK / 2
		    {OV7670_REG_COM7,  0x16}, // QVGA + RGB + Color Bar Enable [7, 8]
		    {OV7670_REG_COM15, 0xD0}, // RGB565 + Full Range [9]

		    // Windowing chuẩn VGA (sau đó Scale xuống QVGA) [10]
		    {OV7670_REG_HSTART, 0x13},
		    {OV7670_REG_HSTOP,  0x01},
		    {OV7670_REG_VSTART, 0x32},
		    {OV7670_REG_VSTOP,  0xbf},
		    {0x32, 0x80},             // HREF Control [11]

		    // Scaling QVGA (320x240)
		    {0x0C, 0x0C}, // COM3: Enable Scale + DCW [12, 13]
		    {0x3E, 0x00}, // COM14: Dùng PCLK mặc định, không chia thêm để tránh lệch dòng [14, 15]
		    {0x72, 0x11}, // Downsample by 2 [16]
		    {0x73, 0x00}, // SCALING_PCLK_DIV: Không chia PCLK [17]

		    // KÍCH HOẠT 8-BAR {71[5], 70[5]} = 10
		    {0x70, 0x3A}, // Bit 7 = 0 [16]
		    {0x71, 0xB5}, // Bit 7 = 1 [16]

		    {0xFF, 0xFE}
};*/
static const ov7670_reg_t ov7670_qvga_rgb565[] = {
		// 1. Reset chip
		    {OV7670_REG_COM7,  0x80},
		    {0xFF, 0xFF},             // Delay chờ camera ổn định

		    // 2. Format: QVGA + RGB565
		    {0x12, 0x14},             // COM7: QVGA, RGB
		    {0x8C, 0x00},             // RGB444 Disable
		    {0x40, 0xD0},             // COM15: RGB565, Full range [00..FF]
		    {0x3A, 0x0C},             // TSLB: Set UYVY sequence cho DSP RGB
		    {0x3D, 0x80},             // COM13: Enable Gamma, UV Auto Adjust
		    {0xB0, 0x84},             // 🔥 MAGIC REG: Bật YUV-to-RGB Matrix DSP (Sửa màu xám)

		    // 3. Clock & Hardware Scaling (VGA -> QVGA)
		    {0x0C, 0x04},             // COM3: DCW enable
		    {0x3E, 0x19},             // COM14: Manual scaling, PCLK /= 2
		    {0x70, 0x3A},             // SCALING_XSC
		    {0x71, 0x35},             // SCALING_YSC
		    {0x72, 0x11},             // SCALING_DCWCTR (Downsample by 2)
		    {0x73, 0xF1},             // SCALING_PCLK_DIV (DSP clock /= 2)

		    // 4. Windowing chuẩn 640 PCLKs
		    {0x17, 0x16},             // HSTART
		    {0x18, 0x04},             // HSTOP
		    {0x32, 0x80},             // HREF
		    {0x19, 0x03},             // VSTART
		    {0x1A, 0x7B},             // VSTOP
		    {0x03, 0x0A},             // VREF (Bit thấp VSTART/VSTOP)

			// 5. 🔥 Ma trận màu TƯƠI & CÂN BẰNG ẤM (Trị dứt điểm xanh xám)
					    {0x4F, 0xa0},             // MTX1: Đẩy Red Gain từ 0x80 -> 0xB0 (Giúp màu da, màu đỏ tươi bói)
					    {0x50, 0x9D},             // MTX2: Cắt bớt phần dư thừa của kênh Green
					    {0x51, 0x00},             // MTX3
					    {0x52, 0x20},             // MTX4: Giữ sắc Blue ở mức vừa phải
					    {0x53, 0x82},             // MTX5
					    {0x54, 0x9C},             // MTX6
					    {0x58, 0x9E},             // MTXS: Auto Contrast Sign
					    {0x4C, 0x70},             // 🔥 SATCTR: Tăng Saturation kịch khung (0x70) giúp màu đậm đà, tươi tắn
						// 🔥 CÂN BẰNG LẠI AWB: Giảm Red Gain để không bị đỏ khi gặp đèn vàng
						    {0x13, 0xE7},             // COM8: Bật Fast AEC, AWB, AGC
						    {0x01, 0x50},             // BLUE Gain: Đặt ở mức 0x50 (Vừa đủ tươi)
						    {0x02, 0x4E},             // RED Gain: Hạ từ 0x68 -> 0x4E (Giảm đỏ dư thừa)

						    // Khung giới hạn thuật toán AWB tự động căn chỉnh
						    {0x6A, 0x40},             // AWBC1
						    {0x6B, 0x0A},             // AWBC2
						    {0x6C, 0x0A},             // AWBC3
						    {0x6D, 0x55},             // AWBC4
						    {0x6E, 0x11},             // AWBC5
						    {0x6F, 0x9E},             // AWBC6

		    // 6. Tối ưu ảnh (Edge Enhancement & De-noise)
		    {0x41, 0x38},             // COM16: Edge enhancement, De-noise, AWB gain

		    // 7. 🔥 Bảng Gamma Curve (Tăng độ tương phản & độ tươi màu)
		    {0x7B, 16},
		    {0x7C, 30},
		    {0x7D, 53},
		    {0x7E, 90},
		    {0x7F, 105},
		    {0x80, 118},
		    {0x81, 130},
		    {0x82, 140},
		    {0x83, 150},
		    {0x84, 160},
		    {0x85, 180},
		    {0x86, 195},
		    {0x87, 215},
		    {0x88, 230},
		    {0x89, 244},
		    {0x7A, 16},

		    // 8. Clock Prescaler & Lật hình
		    {0x11, 0x01},             // CLKRC: Pre-scalar = 1/1
		    {0x1E, 0x31},             // MVFP: Mirror + Flip hình đúng chiều

		    {0xFF, 0xFE}              // Kết thúc cấu hình
};
uint8_t OV7670_Write_Reg(I2C_RegDef_t *i2cx, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    I2C_Master_Transmit(i2cx, buf, 2, OV7670_I2C_ADDR);
    return 0;
}

uint8_t OV7670_Read_Reg(I2C_RegDef_t *i2cx, uint8_t reg, uint8_t *data)
{
    I2C_Master_Transmit(i2cx, &reg, 1, OV7670_I2C_ADDR);
    I2C_Master_Receive(i2cx, data, 1, OV7670_I2C_ADDR);
    return 0;
}

uint8_t OV7670_Init(I2C_RegDef_t *i2cx)
{
    uint8_t id = 0;

    // 1. Check PID
    OV7670_Read_Reg(i2cx, 0x0A, &id);
    if (id != 0x76) return 1; // Lỗi I2C hoặc không thấy camera

    // 2. Load mảng cấu hình
    uint16_t i = 0;
    while (1)
    {
        if (ov7670_qvga_rgb565[i].reg == 0xFF && ov7670_qvga_rgb565[i].val == 0xFE) break;

        if (ov7670_qvga_rgb565[i].reg == 0xFF && ov7670_qvga_rgb565[i].val == 0xFF)
        {
            for (volatile int d = 0; d < 120000; d++); // Delay Reset
        }
        else
        {
            OV7670_Write_Reg(i2cx, ov7670_qvga_rgb565[i].reg, ov7670_qvga_rgb565[i].val);
            for (volatile int d = 0; d < 12000; d++); // Delay
        }
        i++;
    }

    return 0;
}
