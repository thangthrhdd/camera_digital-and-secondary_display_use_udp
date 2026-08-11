# STM32F4 Digital Camera Project

A embedded digital camera project built on the **STM32F407** microcontroller. Features real-time live preview, photo capture to SD card, user navigation via Joystick, and image playback on an ST7789 TFT display.

<p align="center">
  <img width="500" alt="STM32F4 Digital Camera Setup" src="https://github.com/user-attachments/assets/29df12bd-1fe8-4cc1-81f7-9d79316af1ed" />
</p>
<img width="1184" height="1034" alt="08642d1e-2602-424f-9d10-08d034f2a617" src="https://github.com/user-attachments/assets/8640931c-3f43-4db8-a8fa-4cd694b4b0c4" />


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
| `PC9` | XCLK |
| `PA6` | PCLK |
| `PA4` | HS|
| `PA7` | VS|
| `PB10` | SCL | SCCB I2C CLock |
| `PB11` | SDA | SCCB I2C Data  |
| `PA9` | D0 | DVP Data Lines |
| `PA10`| D1 | DVP Data Lines |
| `PC8` | D2 | DVP Data Lines |
| `PE1` | D3 | DVP Data Lines |
| `PC11`| D4 | DVP Data Lines |
| `PB6` | D5 | DVP Data Lines |
| `PB8` | D6 | DVP Data Lines |
| `PB9` | D7 | DVP Data Lines |


### 2. ST7789 Display & Joystick Interface & SD_CARD
| STM32F4 Pin | Peripheral Pin | Function |
| :---: | :---: | :--- |
| `PB3` | ST7789 SCK | SPI Clock |
| `PB4` | ST7789 MOSI | SPI Master Out |
| `PB5` | ST7789 MISO | SPI Master IN  |
| `PD5` | ST7789 CS |
| `PD6` | ST7789 DC |
| `PD7` | ST7789 RST|
| `PB13`| SD_CARD SCK | SPI Clock |
| `PC3` | SD_CARD MOSI | SPI Master Out |
| `PC2` | SD_CARD MISO | SPI Master IN  |
| `PB12`| SD_CARD CS |
| `PA2` | Joystick VRy | Analog  |
| `PB0` | Joystick VRx | Analog  |
| `PD0` | SW | Select |
| `PB1` | ADC_Battery manager|

---

##  How to Run
1. Insert a FAT32-formatted SD Card (8GB - 16GB).
2. Flash the firmware to STM32F407 using **STM32CubeIDE** or **ST-Link Utility**.
3. Power up the board, use the **Joystick** to toggle between Live Preview and Playback mode.
