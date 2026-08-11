# STM32F4 Digital Camera Project

A embedded digital camera project built on the **STM32F407** microcontroller. Features real-time live preview, photo capture to SD card, user navigation via Joystick, and image playback on an ST7789 TFT display.

<p align="center">
  <img width="500" alt="STM32F4 Digital Camera Setup Overview" src="https://github.com/user-attachments/assets/29df12bd-1fe8-4cc1-81f7-9d79316af1ed" />
</p>

<p align="center">
  <img width="600" alt="Detailed Hardware Setup and Wiring" src="https://github.com/user-attachments/assets/8640931c-3f43-4db8-a8fa-4cd694b4b0c4" />
</p>

---

## Technical Specifications

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
## Block diagram
<img width="1024" height="602" alt="image" src="https://github.com/user-attachments/assets/01e869b0-3c37-4073-a9d7-cdf5416788b1" />

## Hardware Connections (Pinout Mapping)

### 1. OV7670 Camera Interface
| STM32F4 Pin | OV7670 Pin | Description |
| :---: | :---: | :--- |
| `PC9` | XCLK | System Clock Out |
| `PA6` | PCLK | Pixel Clock |
| `PA4` | HS | Horizontal Sync |
| `PA7` | VS | Vertical Sync |
| `PB10` | SCL | SCCB I2C Clock |
| `PB11` | SDA | SCCB I2C Data |
| `PA9` | D0 | DVP Data Line 0 |
| `PA10`| D1 | DVP Data Line 1 |
| `PC8` | D2 | DVP Data Line 2 |
| `PE1` | D3 | DVP Data Line 3 |
| `PC11`| D4 | DVP Data Line 4 |
| `PB6` | D5 | DVP Data Line 5 |
| `PB8` | D6 | DVP Data Line 6 |
| `PB9` | D7 | DVP Data Line 7 |

### 2. ST7789 Display & Joystick Interface & SD_CARD
| STM32F4 Pin | Peripheral Pin | Function |
| :---: | :---: | :--- |
| `PB3` | ST7789 SCK | SPI Clock |
| `PB4` | ST7789 MOSI | SPI Master Out |
| `PB5` | ST7789 MISO | SPI Master IN (Optional) |
| `PD5` | ST7789 CS | Chip Select |
| `PD6` | ST7789 DC | Data/Command |
| `PD7` | ST7789 RST | Reset |
| `PB13`| SD_CARD SCK | SPI Clock |
| `PC3` | SD_CARD MOSI | SPI Master Out |
| `PC2` | SD_CARD MISO | SPI Master IN |
| `PB12`| SD_CARD CS | Chip Select |
| `PA2` | Joystick VRy | Analog Input Y-axis |
| `PB0` | Joystick VRx | Analog Input X-axis |
| `PD0` | Joystick SW | Select / Shutter Button |
| `PB1` | ADC_Battery manager| Analog Input Battery voltage |

---

##  How to Run
1. Insert a FAT32-formatted SD Card (8GB - 16GB).
2. Open the project and build `camtest.c` located in the `Src/` directory.
3. Flash the firmware to STM32F407 using **STM32CubeIDE** or **ST-Link Utility**.
4. Power up the board, use the **Joystick** to toggle between Live Preview and Playback mode.


#  STM32F4 Secondary Display use Ethernet (UDP)

An embedded secondary display project built on the **STM32F407** microcontroller. Features real-time frame buffer streaming over Ethernet using the **UDP protocol** to display video/screen feeds on an ST7789 TFT LCD.

---

##  Key Features

* **Real-time UDP Streaming:** Receives and renders display frame buffers directly over Ethernet using a lightweight UDP packet parser.
* **TFT LCD Rendering:** Fast SPI transfer and rendering on an **ST7789** display module (QVGA 320x240 resolution).
* **Hardware Controls:** **Joystick** support for navigating menus, adjusting display modes, and controlling stream settings.
* **Battery Level Sensing:** Integrated ADC voltage divider circuit for real-time battery level monitoring.

---

##  Key Hardware Components

* **Microcontroller:** STM32F407VGT6 (ARM Cortex-M4 Core @ 168 MHz)
* **Ethernet PHY:** DP83848 / LAN8720 (RMII Interface)
* **Display Module:** ST7789 TFT LCD Controller (SPI2 Interface)
* **User Controls:** 5-axis Analog Joystick (ADC & GPIO)
* **Power Monitoring:** Resistor Voltage Divider ($R_1 / R_2$) connected to `ADC (Vbat)`

---

##  Hardware Connections (Pinout Mapping)

### 1. Ethernet PHY Interface (RMII Protocol)
| STM32F4 Pin | Peripheral Signal | Configuration / Function | Mode / AF |
| :---: | :---: | :--- | :---: |
| `PA1` | ETH_RMII_REF_CLK | Reference Clock Input | AF11 |
| `PA2` | ETH_MDIO | Management Data Input/Output | AF11 |
| `PA7` | ETH_RMII_CRS_DV | Carrier Sense / Data Valid | AF11 |
| `PB11` | ETH_RMII_TX_EN | Transmit Enable | AF11 |
| `PB12` | ETH_RMII_TXD0 | Transmit Data Line 0 | AF11 |
| `PB13` | ETH_RMII_TXD1 | Transmit Data Line 1 | AF11 |
| `PC1` | ETH_MDC | Management Data Clock | AF11 |
| `PC4` | ETH_RMII_RXD0 | Receive Data Line 0 | AF11 |
| `PC5` | ETH_RMII_RXD1 | Receive Data Line 1 | AF11 |

---

### 2. ST7789 Display Interface (SPI1)
| STM32F4 Pin | Peripheral Pin | Function / Description | Mode / AF |
| :---: | :---: | :--- | :---: |
| `PB3` | SCK | SPI1 Clock | AF5 |
| `PB4` | MISO / MOSI | SPI1 Data Line | AF5 |
| `PB5` | MOSI / MISO | SPI1 Data Line | AF5 |
| `PB6` | DC (RS) | Data / Command Select Line | Output (Pull-Up) |
| `PB7` | RST | Hardware Reset Line | Output (Pull-Up) |
| `PB9` | CS | Chip Select Line | Output (Pull-Up) |

---

### 3. SD Card / Secondary SPI Interface (SPI2)
| STM32F4 Pin | Peripheral Pin | Function / Description | Mode / AF |
| :---: | :---: | :--- | :---: |
| `PB10` | SCK | SPI2 Clock Line | AF5 |
| `PC2` | MISO | SPI2 Master In Slave Out | GPIO / Alternate |
| `PC3` | MOSI | SPI2 Master Out Slave In | GPIO / Alternate |
| `PD2` | CS | Chip Select  GPIO / Alternate |

---

### 4. ADC & User Controls (Joystick, Battery, EXTI)
| STM32F4 Pin | Peripheral Channel | Function / Description | Mode / Configuration |
| :---: | :---: | :--- | :---: |
| `PA3` | ADC Channel | Joystick VRy (Analog Input) | Analog Mode |
| `PA4` | ADC Channel | Joystick VRx (Analog Input) | Analog Mode |
| `PD1` | EXTI Line 1 | Joystick Switch / Button Interrupt | EXTI Rising Trigger (Pull-Up) |


##  How to Run

1. Connect the STM32F4 board to your local network/PC using an RJ45 Ethernet cable.
2. Build and flash the firmware (`tfteth.c` / UDP secondary display module) using **STM32CubeIDE** or **ST-Link Utility**.
3. Run the Python / C++ UDP video streaming script on your PC to start broadcasting frames over the network.
4. Power up the board to view the real-time secondary display stream on the ST7789 LCD!
