/*
 * menu1.h
 *
 *  Created on: Jul 23, 2026
 *      Author: ADMIN
 */

#ifndef INC_MENU1_H_
#define INC_MENU1_H_



#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f407.h"
#include "gpio.h"
#include "spi.h"
#include "tftcolor.h"
#include "ff.h"
#include "sd.h"
#include "eth.h"
#include"ov7670.h"
#include "dcmi.h"

// Chế độ hệ thống chính
typedef enum {
    MODE_SD = 0,
    MODE_ETH,
	MODE_CAM
} SystemMode_t;

// Chế độ con trong SD Mode
typedef enum {
    SD_APP_MENU = 0,
    SD_APP_PLAY_RAW
} SDAppMode_t;

// Pointer cho hàm Action
typedef void (*ActionFunc_t)(void);
typedef void (*App_state_t)(MenuEvent_t input, SPI_Handle_t *hspi);

typedef struct MenuNode
{
    char * title;                  // Chứa đường dẫn/tên file (VD: "gai.RAW")
    struct MenuNode * parent;
    struct MenuNode * child;
    struct MenuNode * sibling_up;
    struct MenuNode * sibling_down;
    ActionFunc_t action;
} MenuNode;

#define Sibling 1
#define Children 0

// Biến quản lý trạng thái
extern volatile SystemMode_t current_sys_mode;
extern volatile SDAppMode_t current_sd_app;
extern volatile uint8_t read_active;

extern MenuNode *root;
extern MenuNode *current_node;
extern volatile App_state_t current_app;
extern SPI_Handle_t spi1,spi;
// Prototypes
MenuNode* New_Node(char * title, ActionFunc_t action);
MenuNode *Add_Node(char* title, MenuNode * target_node, int mode, ActionFunc_t action);
void Menu_Init_Tree(void);
void Menu_Render(SPI_Handle_t* hspi);

void App_PlayVideo_Action(void);
void App_Ethernet_Action(void);
void App_Camera_Actiton(void);
void  App_Camera_SNAP_SHOT_Actiton(void);
App_state_t Menu_Event(MenuEvent_t input, SPI_Handle_t *hspi);
App_state_t ETH_Handle(MenuEvent_t input, SPI_Handle_t *hspi);
App_state_t SD_IMGE_Handle(MenuEvent_t input, SPI_Handle_t *hspi);
App_state_t CAM_Handlde(MenuEvent_t input, SPI_Handle_t *hspi);
App_state_t CAM_SNAP_SHOT_Handlde(MenuEvent_t input, SPI_Handle_t *hspi);
void menu_handle(MenuEvent_t input, SPI_Handle_t* hspi);
#endif /* INC_MENU1_H_ */
