# STM32F4 Digital Camera Project

A embedded digital camera project built on the **STM32F407** microcontroller. Features real-time live preview, photo capture to SD card, user navigation via Joystick, and image playback on an ST7789 TFT display.

<p align="center">
  <img width="500" alt="STM32F4 Digital Camera Setup" src="https://github.com/user-attachments/assets/29df12bd-1fe8-4cc1-81f7-9d79316af1ed" />
</p>

---

##  Technical Specifications

| Category | Specification & Features |
| :--- | :--- |
| **Photo Capture** | • Live View preview: **~20 FPS**<br>• Resolution: **QVGA (320x240)**<br>• Format: **RAW (`*.RAW`)** |
| **Playback Mode** | • Format: **RGB565 (320x240)** |
| **Storage Media** | • SD Card (**FAT32** file system)<br>• Recommended capacity: **8GB – 16GB** |
| **User Interface** | • 5-axis / Analog **Joystick** (Navigation, Shutter & Playback control) |

---

##  Key Hardware Components

* **Microcontroller Board:** STM32F407VGT6 (ARM Cortex-M4 Core @ 168 MHz)
* **Camera Module:** OV7670 CMOS Camera Module
* **Display Module:** ST7789 TFT LCD Controller (320x240)
* **User Controls:** Joystick Module (Up/Down/Left/Right/Select)
* **Storage:** SD Card Reader Module (SDIO / SPI)

---

##  Hardware Connections (Pinout Mapping)


### 1. OV7670 Camera Interface
| STM32F4 Pin | OV7670 Pin | Description |
| :---: | :---: | :--- |
| `PA8` | XCLK | System Clock Out |
| `PB6` | SCL | SCCB I2C Clock |
| `PB7` | SDA | SCCB I2C Data |
| `PC6-PC11` | D0 - D5 | DVP Data Lines |

### 2. ST7789 Display & Joystick Interface
| STM32F4 Pin | Peripheral Pin | Function |
| :---: | :---: | :--- |
| `PA5` | ST7789 SCL | SPI Clock |
| `PA7` | ST7789 SDA | SPI Master Out |
| `PB0` | Joystick VRx/VRy | Analog / Digital Navigation |
| `PB1` | Joystick SW | Shutter / Select Button |

---

##  How to Run
1. Insert a FAT32-formatted SD Card (8GB - 16GB).
2. Flash the firmware to STM32F407 using **STM32CubeIDE** or **ST-Link Utility**.
3. Power up the board, use the **Joystick** to toggle between Live Preview and Playback mode.
