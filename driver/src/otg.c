/*
 * otg.c
 *
 *  Created on: Nov 18, 2025
 *      Author: ADMIN
 */
#include"otg.h"
#define BLOCK_SIZE  6400 // 320 pixel * 10 dòng * 2 bytes

volatile uint8_t rx_buf_A[BLOCK_SIZE];
volatile uint8_t rx_buf_B[BLOCK_SIZE];

// Con trỏ quản lý: USB nạp vào đâu, Main vẽ từ đâu
uint8_t* usb_rx_ptr = rx_buf_A;           // Ban đầu USB nạp vào mảng A
volatile uint8_t* main_draw_ptr = NULL;  // Con trỏ chứa dữ liệu tĩnh cho Main vẽ

uint32_t rx_index = 0;
volatile uint8_t rx_ready = 0; // Cờ báo cho main() biết đệm đã đầy
extern uint32_t packet[64];
extern USB_SETUP_PACKET op,pp;
extern uint8_t mode,pktsts,count,count1,key,rei;
extern uint16_t  bcnt,addr,addr1;
extern uint8_t receive[64];
static uint8_t device_descriptor[18] = {

		   0x12,       // bLength
		   0x01,       // bDescriptorType (Device)
		   0x00, 0x02, // bcdUSB 2.00
		   0x02,       // bDeviceClass
		   0x00,       // bDeviceSubClass
		   0x00,       // bDeviceProtocol
		   0x40,       // bMaxPacketSize0 = 64
		   0x83, 0x04, // idVendor (VID)
		   0x40, 0x57, // idProduct (PID)
		   0x00, 0x01, // bcdDevice
		   0x00,       // iManufacturer
		   0x00,       // iProduct
		   0x00,       // iSerialNumber
		   0x01        // bNumConfigurations

 };
static uint8_t config_descriptor[67]=
{
	    0x09,       // bLength
	    0x02,       // bDescriptorType (Configuration)
	    0x43, 0x00, // wTotalLength = 67 bytes
	    0x02,       // bNumInterfaces
	    0x01,       // bConfigurationValue
	    0x00,       // iConfiguration
	    0xC0,       // bmAttributes (Self Powered)
	    0x32,       // bMaxPower (100mA)

	    // ----- Interface 0: CDC Communication -----
	    0x09,       // bLength
	    0x04,       // bDescriptorType (Interface)
	    0x00,       // bInterfaceNumber
	    0x00,       // bAlternateSetting
	    0x01,       // bNumEndpoints (1 interrupt IN)
	    0x02,       // bInterfaceClass (CDC)
	    0x02,       // bInterfaceSubClass (ACM)
	    0x01,       // bInterfaceProtocol (AT commands)
	    0x00,       // iInterface

	    // Header Functional Descriptor
	    0x05,       // bFunctionLength
	    0x24,       // bDescriptorType (CS_INTERFACE)
	    0x00,       // bDescriptorSubtype (Header)
	    0x10, 0x01, // bcdCDC = 1.10

	    // Call Management Functional Descriptor
	    0x05,       // bFunctionLength
	    0x24,       // bDescriptorType (CS_INTERFACE)
	    0x01,       // bDescriptorSubtype (Call Management)
	    0x00,       // bmCapabilities (no call mgmt)
	    0x01,       // bDataInterface = 1

	    // ACM Functional Descriptor
	    0x04,       // bFunctionLength
	    0x24,       // bDescriptorType (CS_INTERFACE)
	    0x02,       // bDescriptorSubtype (Abstract Control Management)
	    0x02,       // bmCapabilities (Set_Line_Coding, etc.)

	    // Union Functional Descriptor
	    0x05,       // bFunctionLength
	    0x24,       // bDescriptorType (CS_INTERFACE)
	    0x06,       // bDescriptorSubtype (Union)
	    0x00,       // bMasterInterface = 0
	    0x01,       // bSlaveInterface0 = 1

	    // Endpoint (Interrupt IN, EP1 IN)
	    0x07,
	    0x05,
	    0x81,       // IN, EP1
	    0x03,       // Interrupt
	    0x08, 0x00, // 8 bytes
	    0x10,       // Interval 16ms

	    // ----- Interface 1: CDC Data -----
	    0x09,
	    0x04,
	    0x01,       // bInterfaceNumber
	    0x00,
	    0x02,       // 2 endpoints
	    0x0A,       // Data Interface Class
	    0x00,
	    0x00,
	    0x00,

	    // Endpoint (Bulk OUT, EP2 OUT)
	    0x07,
	    0x05,
	    0x02,       // OUT, EP2
	    0x02,       // Bulk
	    0x40, 0x00, // 64 bytes
	    0x00,

	    // Endpoint (Bulk IN, EP3 IN)
	    0x07,
	    0x05,
	    0x83,       // IN, EP3
	    0x02,       // Bulk
	    0x40, 0x00, // 64 bytes
	    0x00
};
extern uint8_t set_line[7];
void CDC_SendData(OTG_FS_ConFig_t*otg_fs,uint8_t*data,uint16_t length)
{
	otg_fs->Device->DIEP[3].DIEPTSIZ=(1<<19)|length;
	otg_fs->Device->DIEP[3].DIEPCTL|=(1<<31)|(1<<26);
	while(otg_fs->Device->DIEP[3].DTXFSTS<((length+3)/4));
	uint8_t row=length/4;
	uint8_t col=length%4;
	for(int i=0;i<row;i++)
	{
		uint32_t tmp3=0;
		for(uint8_t j=0;j<4;j++)
		{
	        tmp3|=*data<<(j*8);
	        data++;
		}
	*((__vo  uint32_t*)(0x50004000+0x40*4+i*4))=tmp3;
	}
	uint32_t tmp3=0;
	for(int i=0;i<col;i++)
	{
		tmp3|=*data<<(i*8);
		data++;
	}
	*((__vo  uint32_t*)(0x50004000+0x40*4+row*4))=tmp3;
}
static void OTG_Stop_Device(OTG_FS_ConFig_t*otg_fs)
{
	  /* Clear Pending interrupt */
	  for (uint8_t i = 0; i < 4; i++)
	  {
	    otg_fs->Device->DIEP[i].DIEPINT = 0xFB7F;
	    otg_fs->Device->DOEP[i].DOEPINT = 0xFB7F;
	  }

	  /* Clear interrupt masks */
	  otg_fs->Device->DIEPMSK  = 0U;
	  otg_fs->Device->DOEPMSK  = 0U;
	  otg_fs->Device->DAINTMSK = 0U;

	  /* Flush the FIFO */


	  while(!(otg_fs->Global->GRSTCTL&(1<<31))); /* Wait for AHB master IDLE state. */
	  otg_fs->Global->GRSTCTL=(0x10<<6)|1<<5;/*Flush the TXFIFO*/
	  while((otg_fs->Global->GRSTCTL&(1<<5)));

	  while(!(otg_fs->Global->GRSTCTL&(1<<31))); /* Wait for AHB master IDLE state.*/

	  otg_fs->Global->GRSTCTL=1<<4;/*Flush the RXFIFO*/
	  while((otg_fs->Global->GRSTCTL&(1<<4)));
}
void OTG_FS_Core_Init(OTG_FS_ConFig_t*otg_fs)
{
	OTG_PCLK_EN();
	NVIC_ConFig(67,ENABLE);
	NVIC_PRIORITY(67,1);

	OTG_Stop_Device(otg_fs);
	while(!(otg_fs->Global->GRSTCTL&(1<<31))); /* Wait for AHB master IDLE state. */
	otg_fs->Global->GRSTCTL |= 1<<0;
	while(otg_fs->Global->GRSTCTL & (1<<0));

	//1. Program the following fields in the OTG_FS_GAHBCFG register:
	otg_fs->Global->GCCFG|=1<<16;
	otg_fs->Global->GAHBCFG|=1;//Global interrupt mask bit GINTMSK = 1
	otg_fs->Global->GINTMSK|=1<<4;
	otg_fs->Global->GAHBCFG|=1<<8;//Periodic TxFIFO empty level

	//2. Program the following fields in the OTG_FS_GUSBCFG register:

	otg_fs->Global->GUSBCFG&=~(1<<9);//HNP capable bit
	otg_fs->Global->GUSBCFG&=~(1<<8);//SRP capable bit

	otg_fs->Global->GUSBCFG&=~(0x03);//FS timeout calibration
	otg_fs->Global->GUSBCFG|=0x00;

	otg_fs->Global->GUSBCFG&=~(0x0f<<10);//USB turnaround time field
	otg_fs->Global->GUSBCFG|=(0x06<<10);//18mhz

	//3. The software must unmask the following bits in the OTG_FS_GINTMSK register:
	otg_fs->Global->GINTMSK|=(1<<2)|(1<<1);//OTG interrupt mask
            //Mode mismatch interrupt mask
	 otg_fs->Device->DCFG=0;
	otg_fs->Device->DCTL|=1<<1;
	for(int i=0;i<1000;i++);
	otg_fs->Device->DCTL&=~(1<<1);

}
void OTG_FS_Device_Init(OTG_FS_ConFig_t*otg_fs)
{
	//OTG_PCLK_EN();
	//1. Program the following fields in the OTG_FS_DCFG register:
	//– Device speed
	//– Non-zero-length status OUT handshake
	//otg_fs->Device->DCFG&=~(0x7f<<4);
	 otg_fs->Global->GUSBCFG |= (1 << 30); // FDMOD = 1 (Force Device Mode)
	 for (int i = 0; i <5000; i++); // delay ~25µs để core chuyển mode

	otg_fs->Device->DCFG|=0x03;
	otg_fs->Device->DCFG&=~(0x01<<2);
	/*2. Program the OTG_FS_GINTMSK register to unmask the following interrupts:
        – USB reset
        – Enumeration done
        – Early suspend
        – USB suspend
        – SOF*/
	otg_fs->Global->GINTMSK|=(1<<12)|(1<<13)|(1<<10)|(1<<11)|(1<<3);
	/*3. Program the VBUSBSEN bit in the OTG_FS_GCCFG register to enable VBUS sensing
       in “B” device mode and supply the 5 volts across the pull-up resistor on the DP line.*/
	//otg_fs->Global->GCCFG&=~(1<<21);
	otg_fs->Global->GCCFG|=1<<21|1<<20|1<<19;
}
void  USB_Handle_Setup(OTG_FS_ConFig_t*otg_fs,USB_SETUP_PACKET*sp)
{
	switch(sp->bmRequestType)
	{
	case 0x00:
		switch (sp->bRequest)
		{
		case 0x05:
			addr=sp->wValue;
			//otg_fs->Global->GRSTCTL = (1 << 4);
			//while (otg_fs->Global->GRSTCTL & (1 << 4));

			otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
			otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
			otg_fs->Device->DCFG&=~(0x7f<<4);
			otg_fs->Device->DCFG|=(addr<<4);
			break;
		case 0x09:
			if(sp->wValue==0x01)
			{
				otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
				otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
			}
			break;

		default:
			break;
		}
		break;
		case 0x02: //clear_feature
			if(sp->bRequest==0x01 )
			{
				if(sp->wIndex==0x81)
				{
				otg_fs->Device->DAINTMSK|=(0x1<<1);
				otg_fs->Global->DIEPTXF[0]=(64<<16)|0x40;
				 // 1. CLEAR STALL
				    otg_fs->Device->DIEP[1].DIEPCTL &= ~(1<<21);

				    // 2. Reset DATA PID to DATA0
				    otg_fs->Device->DIEP[1].DIEPCTL |= (1<<28);
				//ep1 interrupt
		    	otg_fs->Device->DIEP[1].DIEPCTL&=~(0x03<<18|0x7ff);
		    	otg_fs->Device->DIEP[1].DIEPCTL|=(1<<15)|(0x03<<18)|8;

		    	//otg_fs->Device->DIEP[1].DIEPCTL|=(1<<31)|(1<<26);

				}
				else if(sp->wIndex==0x2)
				{
					otg_fs->Device->DAINTMSK|=(0x1<<18);
					 // 1. CLEAR STALL
					    otg_fs->Device->DOEP[2].DOEPCTL &= ~(1<<21);

					    // 2. Reset DATA PID to DATA0
					    otg_fs->Device->DOEP[2].DOEPCTL |= (1<<28);

			    	otg_fs->Device->DOEP[2].DOEPCTL&=~(0x03<<18|0x7ff);
			    	otg_fs->Device->DOEP[2].DOEPCTL|=(1<<31)|(1<<15)|(1<<26)|(0x02<<18)|64;
				}
				else if(sp->wIndex==0x83)
				{
					otg_fs->Device->DAINTMSK|=(0x01<<3);
					otg_fs->Global->DIEPTXF[2]=(64<<16)|0x40;

					 // 1. CLEAR STALL
					    otg_fs->Device->DIEP[3].DIEPCTL &= ~(1<<21);

					    // 2. Reset DATA PID to DATA0
					    otg_fs->Device->DIEP[3].DIEPCTL |= (1<<28);

    				//ep3 IN bulk
            	    otg_fs->Device->DIEP[3].DIEPCTL&=~(0x03<<18|0x7ff);
    	            otg_fs->Device->DIEP[3].DIEPCTL|=(1<<15)|(0x02<<18)|64;

					otg_fs->Device->DAINTMSK|=(0x1<<18);
					 // 1. CLEAR STALL
					    otg_fs->Device->DOEP[2].DOEPCTL &= ~(1<<21);

					    // 2. Reset DATA PID to DATA0
					    otg_fs->Device->DOEP[2].DOEPCTL |= (1<<28);

			    	otg_fs->Device->DOEP[2].DOEPCTL&=~(0x03<<18|0x7ff);
			    	otg_fs->Device->DOEP[2].DOEPCTL|=(1<<15)|(0x02<<18)|64;

			    	otg_fs->Device->DOEP[2].DOEPTSIZ=(1<<19)|6400;
			    	otg_fs->Device->DOEP[2].DOEPCTL|=(1<<31)|(1<<26);

			    	key=1;
				}
				otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
				otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
			}
			break;
		case 0x21:
			if(sp->bRequest==0x20)
			{
				otg_fs->Device->DOEP[0].DOEPTSIZ=(1<<19)|7|(1<<29);
				otg_fs->Device->DOEP[0].DOEPCTL|=(1<<31)|(1<<26);
			}
			else if(sp->bRequest==0x22)
			{
				otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
				otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
			}
			break;
		case 0x80:
			switch (sp->bRequest)
			{
			case 0x06:
				switch(sp->wValue>>8)
				{
				case 0x01:
					count++;
					otg_fs->Global->GRSTCTL = (1 << 4);
					while (otg_fs->Global->GRSTCTL & (1 << 4));

					otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|18;
					otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
					while(otg_fs->Device->DIEP[0].DTXFSTS<5);
					for(int i=0;i<16;i+=4)
					{
						uint32_t tmp=0;

						for(int j=0;j<4;j++)
						{
							tmp|=(uint32_t)(device_descriptor[j+i])<<(j*8);
						}
						*((__vo  uint32_t*)(0x50001000+0x40*4 +i))=tmp;
					}
					uint32_t tmp=0;
					tmp=(uint32_t)(device_descriptor[16])|(uint32_t)(device_descriptor[17]<<8);
					*((__vo  uint32_t*)(0x50001000+0x40*4 +16))=tmp;
					break;
				case 0x02:
					otg_fs->Global->GRSTCTL = (1 << 4);
					while (otg_fs->Global->GRSTCTL & (1 << 4));
					if(sp->wLength>0x09 && sp->wLength<0x1000)
					{
					if(count1==0 )
					{
					otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|64;
					otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
					while(otg_fs->Device->DIEP[0].DTXFSTS<16);
					for(int i=0;i<64;i+=4)
					{
						uint32_t tmp1=0;

						for(int j=0;j<4;j++)
						{
							tmp1|=(uint32_t)(config_descriptor[j+i])<<(j*8);
						}
						*((__vo  uint32_t*)(0x50001000+0x40*4 +i))=tmp1;
					}
					count1=1;
					}
					else if(count1==1)
					{
						otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|3;
						otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
						while(otg_fs->Device->DIEP[0].DTXFSTS<1);
					uint32_t tmp2=0;

					tmp2=(uint32_t)(config_descriptor[64])|(uint32_t)(config_descriptor[65]<<8)|(uint32_t)(config_descriptor[66]<<16);
					*((__vo  uint32_t*)(0x50001000+0x40*4))=tmp2;
					count1=0;
					}
					}
					else if(sp->wLength<=0x09)
					{
						otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|9;
						otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
						while(otg_fs->Device->DIEP[0].DTXFSTS<3);
						for(int i=0;i<8;i+=4)
						{
							uint32_t tmp1=0;

							for(int j=0;j<4;j++)
							{
								tmp1|=(uint32_t)(config_descriptor[j+i])<<(j*8);
							}
							*((__vo  uint32_t*)(0x50001000+0x40*4 +i))=tmp1;
						}
						uint32_t tmp3=0;
						tmp3=(uint32_t)(config_descriptor[8]);
						*((__vo  uint32_t*)(0x50001000+0x40*4+8))=tmp3;
						//count1=-1;
					}
					else if(sp->wLength>=0x1000)
					{
						otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
						otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
					}
					break;
				case 0x06:
					otg_fs->Device->DIEP[0].DIEPCTL|=(1<<21);
					break;
				default:
				break;
				}
				  break;
				default:
				break;
			}
			  break;
			  case 0xa1:
				  if(sp->bRequest==0x21)
				  {
					   //key=0;
						otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|7;
						otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
						while(otg_fs->Device->DIEP[0].DTXFSTS<2);
						uint32_t tmp3=0;
						tmp3=(uint32_t)(set_line[0])|(set_line[1]<<8)|(set_line[2]<<16)|(set_line[3]<<24);
						*((__vo  uint32_t*)(0x50001000+0x40*4))=tmp3;
						tmp3=0;
						tmp3=(uint32_t)(set_line[4])|(set_line[5]<<8)|(set_line[6]<<16);
						*((__vo  uint32_t*)(0x50001000+0x40*4+4))=tmp3;
				  }
			default:
			break;
	}
}
void OTG_IRQ_Handle(OTG_FS_ConFig_t*otg_fs)
  {
    if(otg_fs->Global->GINTSTS&(1<<13))
	{
    	uint32_t reg=otg_fs->Device->DSTS;
		otg_fs->Device->DIEP[0].DIEPCTL&=~(0x3);
		otg_fs->Global->GINTSTS|=(1<<13);
	}
     if(otg_fs->Global->GINTSTS&(1<<12))//usb reset
	{
		for(uint8_t i=0;i<4;i++)
		{
			otg_fs->Device->DOEP[i].DOEPCTL|=1<<27;
		}
		// Flush TX FIFO 0
		otg_fs->Global->GRSTCTL = (0x10 << 6) | (1 << 5);
		while (otg_fs->Global->GRSTCTL & (1 << 5));

		// Flush RX FIFO
		otg_fs->Global->GRSTCTL = (1 << 4);
		while (otg_fs->Global->GRSTCTL & (1 << 4));

		otg_fs->Device->DAINTMSK|=(1<<16)|1;
		otg_fs->Device->DOEPMSK|=(1<<3)|1;
		otg_fs->Device->DIEPMSK|=(1<<3)|1;

		otg_fs->Global->GRXFSIZ=64;
		otg_fs->Global->DIEPTXF0=(64<<16)|0x40;


		otg_fs->Device->DOEP[0].DOEPTSIZ=(1<<19)|8|(1<<29);


		otg_fs->Device->DIEP[0].DIEPCTL&=~(0x3);
		otg_fs->Device->DOEP[0].DOEPCTL&=~(0x3);
		otg_fs->Device->DIEP[0].DIEPCTL|=(1<<15);
		otg_fs->Device->DOEP[0].DOEPCTL|=(1<<31)|(1<<15)|(1<<26);
		otg_fs->Device->DOEP[0].DOEPCTL&=~(0xf<<22);



		otg_fs->Global->GINTSTS|=(1<<12);//clear usb reset

	}
     if(otg_fs->Global->GINTSTS&(1<<4))
	{
		uint32_t reg=otg_fs->Global->GRXSTSP;
		rei=reg&0x0f;
		(void)(otg_fs->Global->GRXSTSPR);
		//otg_fs->Global->GINTMSK&=~(1<<4);
		uint8_t  PKTSTS=(reg>>17)&(0x0f);
		pktsts=PKTSTS;

		uint16_t  BCNT=(reg>>4)&(0x7ff);
		bcnt=BCNT;
		uint8_t CHNUM=reg&0x0f;
		if ( PKTSTS== 0x6&& BCNT ==0x8) // SETUP packet received
		{
		    packet[0] = *((__vo  uint32_t*)0x50001000);
		    packet[1] = *((__vo  uint32_t*)0x50001000);
		}
		else if(BCNT==0x7 && PKTSTS==0x2 && op.bRequest==0x20)
		{
		    packet[4] = *((__vo  uint32_t*)0x50001000);
		    packet[5] = *((__vo  uint32_t*)0x50001000);

		    set_line[0]=packet[4]&0xff;set_line[2]=packet[4]>>16;
		    set_line[1]=(packet[4]>>8)&0xff;set_line[3]=packet[4]>>24;

		    set_line[4]=packet[5]&0xff;set_line[6]=packet[5]>>16;
		    set_line[5]=(packet[5]>>8)&0xff;

			otg_fs->Device->DIEP[0].DIEPTSIZ=(1<<19)|0;
			otg_fs->Device->DIEP[0].DIEPCTL|=(1<<31)|(1<<26);
		}
		else if(PKTSTS==0x02)
				{
					uint16_t reg=(otg_fs->Device->DAINT)&(0xffff);

					if(rei & 2)
					{
					    uint8_t row = bcnt / 4;
					    uint8_t col = bcnt % 4;

					    // Nạp dữ liệu từ FIFO vào con trỏ usb_rx_ptr (đang trỏ tới A hoặc B)
					    for(int i = 0; i < row; i++)
					    {
					        uint32_t bit = *((__vo uint32_t*)0x50003000);
					        usb_rx_ptr[rx_index++] = bit & 0xFF;
					        usb_rx_ptr[rx_index++] = (bit >> 8) & 0xFF;
					        usb_rx_ptr[rx_index++] = (bit >> 16) & 0xFF;
					        usb_rx_ptr[rx_index++] = (bit >> 24) & 0xFF;
					    }
					    if(col > 0)
					    {
					        uint32_t bit = *((__vo uint32_t*)0x50003000);
					        for(int i = 0; i < col; i++)
					        {
					            usb_rx_ptr[rx_index++] = (bit >> (i * 8)) & 0xFF;
					        }
					    }

					    // Khi nạp đủ 1 block 10 dòng (6400 bytes)
					    if(rx_index >= BLOCK_SIZE)
					    {
					        rx_index = 0;

					        // Thực hiện TRÁO ĐỔI BỘ ĐỆM (Ping-Pong Swap) lập tức
					        if (usb_rx_ptr == rx_buf_A)
					        {
					            main_draw_ptr = rx_buf_A; // Chốt mảng A lại giao cho Main vẽ
					            usb_rx_ptr = rx_buf_B;   // Đuổi USB sang nạp gói tiếp theo vào mảng B
					        }
					        else
					        {
					            main_draw_ptr = rx_buf_B; // Chốt mảng B lại giao cho Main vẽ
					            usb_rx_ptr = rx_buf_A;   // Đuổi USB sang nạp gói tiếp theo vào mảng A
					        }

					        rx_ready = 1; // Báo cho vòng lặp Main biết có khối mới tĩnh để vẽ
					    }

					    // RE-ARM NGAY TRONG NGẮT: Không đợi Main vẽ xong, giải phóng USB luôn để PC bắn tiếp tốc độ cao
					    otg_fs->Device->DOEP[2].DOEPTSIZ = (1 << 19) | 6400;
					    otg_fs->Device->DOEP[2].DOEPCTL |= (1 << 31) | (1 << 26);
					}
				}
		//otg_fs->Global->GINTMSK|=(1<<4);
	}
  	if(otg_fs->Global->GINTSTS&(1<<19))//OEPINT
	{
		uint16_t reg=(otg_fs->Device->DAINT>>16)&(0xffff);
		mode=reg;
		//if((reg/2)==0)
			if(otg_fs->Device->DOEP[0].DOEPINT&(1<<3))
			{
			    op.bmRequestType=packet[0]&(0xff);
			    op.bRequest=(packet[0]>>8)&(0xff);
			    op.wValue=(packet[0]>>16)&(0xffff);
			    op.wIndex=packet[1]&(0xffff);
			    op.wLength=(packet[1]>>16)&(0xffff);
			    USB_Handle_Setup(otg_fs,&op);
			    otg_fs->Device->DOEP[0].DOEPINT|=(1<<3);

				otg_fs->Device->DOEP[0].DOEPTSIZ=(1<<19)|8|(1<<29);
				otg_fs->Device->DOEP[0].DOEPCTL|=(1<<31)|(1<<26);
		     }
			else if(otg_fs->Device->DOEP[0].DOEPINT&(1))
			{
				otg_fs->Device->DOEP[0].DOEPINT|=(1);
				otg_fs->Device->DOEP[0].DOEPTSIZ=(1<<19)|8|(1<<29);
				otg_fs->Device->DOEP[0].DOEPCTL|=(1<<31)|(1<<26);
			}
			else if(otg_fs->Device->DOEP[2].DOEPINT&(1))
			{
				//rei=1;
				otg_fs->Device->DOEP[2].DOEPINT|=(1);

				//CDC_SendData(otg_fs,(uint8_t*)("ok\n"),strlen("ok\n"));
		    	otg_fs->Device->DOEP[2].DOEPTSIZ=(1<<19)|6400;
		    	otg_fs->Device->DOEP[2].DOEPCTL|=(1<<31)|(1<<26);

			}

	}
	if(otg_fs->Global->GINTSTS&(1<<18))
	{
		if(otg_fs->Device->DIEP[0].DIEPINT&(1))
		{
    		if(op.bRequest==0x05 && op.bmRequestType==0x00)
			{
				//OTG_Stop_Device(otg_fs);
				//otg_fs->Device->DCFG&=~(0x7f<<4);
				//otg_fs->Device->DCFG|=set_addr(addr);
    			addr1=(otg_fs->Device->DCFG>>4)&(0x7f);

			}
    		else if(op.bRequest==0x06 && op.bmRequestType==0x80)
    		{
    			if(op.wValue==0x200)
    			{
    			if(op.wLength>0x09 && op.wLength<0x1000)
    			{
    				//count1++;
    				if(count1==1)
    				{
    				//pp=op;
    				USB_Handle_Setup(otg_fs,&op);
    				}
    			    //count1=0;
    			}
    			}
    		}
    		//count1=0;
			otg_fs->Device->DIEP[0].DIEPINT|=1;
		}
		else if(otg_fs->Device->DIEP[0].DIEPINT&(1<<3))
		{
			otg_fs->Device->DIEP[0].DIEPINT|=1<<3;
		}
		else if(otg_fs->Device->DIEP[3].DIEPINT&(1))
		{
			uint16_t reg=(otg_fs->Device->DAINT>>16)&(0xffff);
			mode=reg;
			otg_fs->Device->DIEP[3].DIEPINT|=(1);
		}
	}

	if(otg_fs->Global->GINTSTS&(1<<1))
	{
		otg_fs->Global->GINTSTS|=(1<<1);
	}
	if(otg_fs->Global->GINTSTS&(1<<3))
	{
		otg_fs->Global->GINTSTS|=(1<<3);
	}


}
