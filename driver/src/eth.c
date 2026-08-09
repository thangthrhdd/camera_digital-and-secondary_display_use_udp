/*
 * eth.c
 *
 *  Created on: Apr 5, 2026
 *      Author: ADMIN
 */
#include"eth.h"
#include "eth_hanlde_packet.h"
extern uint16_t id2,id3,id1;
void ETH_CLK(ETH_Handle_t*eth_handle,uint8_t enordi)
{
	if(enordi==ENABLE)
	{
	if(eth_handle->eth_mac==ETH_MAC)
	{
		ETH_MAC_PCLK_EN();
		ETH_TX_PCLK_EN();
		ETH_RX_PCLK_EN();
	}
	}
	else
	{
		if(eth_handle->eth_mac==ETH_MAC)
		{
			ETH_MAC_PCLK_DI();
			ETH_TX_PCLK_DI();
			ETH_RX_PCLK_DI();
		}
	}
}
static void ETH_WRITE_PHY(uint8_t reg_phy ,uint8_t mii_reg, uint16_t data)
{
	while(ETH_MAC->MACMIIAR&1);// đợi đến khi hết busy
	ETH_MAC->MACMIIDR=data;//data của dữ liệu cần ghi
	ETH_MAC->MACMIIAR=reg_phy<<11|mii_reg<<6|(data&(0x07<<2))<<2;// gán địa chỉ phy , thanh ghi mong muốn tgrong phy và clock
	ETH_MAC->MACMIIAR|=1; // bật bit này để ghi
	while(ETH_MAC->MACMIIAR&1); // đợi hết busy
}
static uint32_t ETH_PHY_Read(uint8_t reg_phy ,uint8_t mii_reg)
{
	uint32_t tmp=0;
	while(ETH_MAC->MACMIIAR&1); // dợi hết busy
    tmp = ((uint32_t)(reg_phy& 0x1F) << 11) |
          ((uint32_t)(mii_reg & 0x1F) << 6); // ghi địa chỉ phy, địa chỉ thanh ghi mong muốn

    ETH_MAC->MACMIIAR = tmp;
	ETH_MAC->MACMIIAR|=1;// bật ghi
	while(ETH_MAC->MACMIIAR&1);// đợi hết busy
	return ETH_MAC->MACMIIDR&0xffff;// nhận data
}
void ETH_Init(ETH_Handle_t*eth_handle)
{
	///////step 1
	RCC->AHB1RSTR|=1<<25;
	SYSCFG_PCLK_EN();
	if(eth_handle->eth_x.mode==ETH_RMII_MODE) SYSCFG->PMC|=1<<23;
	else SYSCFG->PMC&=~(1<<23);
	RCC->AHB1RSTR&=~(1<<25);

	ETH_CLK(eth_handle,ENABLE);
	eth_handle->eth_dma->DMABMR|=1;
	while (eth_handle->eth_dma->DMABMR&1);
	eth_handle->eth_dma->DMABMR&=~(0x3ffffff<<1);
	eth_handle->eth_dma->DMABMR|=(32<<8|1<<16|1<<25);
	//////////step2
	eth_handle->eth_dma->DMAIER&=~(1<<15|1<<16|1<<3|1<<4|1<<5|1<<7|1<<8|1<<9|1<<10|1<<13);
	eth_handle->eth_dma->DMAIER=1<<16|1<<0|1<<2|1<<6|1<<14|1<<15|1<<7;
	///step3
	eth_handle->eth_dma->DMATDLAR=0x2001C000;
	eth_handle->eth_dma->DMARDLAR=0x2001C800;
	//ETH_CLK(eth_handle,ENABLE);
	ETH_WRITE_PHY(eth_handle->eth_x.phy_addr, 0, 0x8000);
	for(volatile int i=0; i<500000; i++); // Đợi một lúc cho chip PHY khởi động lại sạch sẽ
	while(!(((ETH_PHY_Read(eth_handle->eth_x.phy_addr, 1)>>2)&1) && ((ETH_PHY_Read(eth_handle->eth_x.phy_addr, 1)>>5)&1)));
	id1 = ETH_PHY_Read(eth_handle->eth_x.phy_addr, 31);
	id2 = ETH_PHY_Read(eth_handle->eth_x.phy_addr, 2);
	id3 = ETH_PHY_Read(eth_handle->eth_x.phy_addr, 3);
	uint8_t data =(id1>>2)&0x07,DM,PS;
	if(data==1) {DM=0;PS=0;}
	if(data==2) {DM=0;PS=1;}
	if(data==5) {DM=1;PS=0;}
	if(data==6) {DM=1;PS=1;}
	// 2. Viết hàm nạp vào thanh ghi (thực hiện trong ETH_Init)
	// MACA0LR nạp 4 bytes đầu (Byte 0, 1, 2, 3)
	eth_handle->eth_mac->MACA0LR = (uint32_t)(eth_handle->eth_x.mac_addr & 0xFFFFFFFF);

	// MACA0HR nạp 2 bytes cuối (Byte 4, 5)
	eth_handle->eth_mac->MACA0HR = (uint32_t)((eth_handle->eth_x.mac_addr >> 32) & 0xFFFF)|1<<31;
	eth_handle->eth_mac->MACCR&=~(1<<11|1<<14);
	eth_handle->eth_mac->MACCR|=(DM<<11|PS<<14);
	eth_handle->eth_mac->MACFFR|=1<<31|1<<0;
	eth_handle->eth_mac->MACCR|=1<<2|1<<3;
	/////step 6
	eth_handle->eth_dma->DMAOMR|=1<<1|1<<13;
	ETH_DMA->DMAIER=1<<16|1<<15|1<<6|1<<7;
	eth_handle->eth_dma->DMARPDR = 0;
	//sửa ngày 19/6/2025
	eth_handle->TX_BUFFER->Status&=~(0x03<<22);
	eth_handle->TX_BUFFER->Status|=(0x03<<22);
	/////////---------///////
	eth_handle->tx_state=RUN;
	eth_handle->rx_state=RUN;
}
void ETH_TX_TRANS(ETH_Handle_t*eth_handle,uint8_t * packet,uint16_t length)
{
	uint32_t status = 0;
	switch (eth_handle->tx_state)
	{
	case RUN:
			if((eth_handle->TX_BUFFER->Status&(1<<31)))
				eth_handle->tx_state=SUPSPEND;
			else
			{
				eth_handle->TX_BUFFER->Buffer1Addr=packet;
				eth_handle->TX_BUFFER->ControlBufferSize=length;
				//eth_handle->tx_state=COMPLE;
				eth_handle->TX_BUFFER->Status |= (1UL << 31) | (1UL << 29) | (1UL << 28);
		        // Đánh thức DMA nếu nó đang ở trạng thái Suspend (Bước 9 trong quy trình)
		        eth_handle->eth_dma->DMATPDR = 0;
				eth_handle->tx_state=RUN;
			}
		break;
	case SUPSPEND:
        // Trạng thái này thường dùng để xử lý lỗi TU (Transmit Buffer Unavailable)
        // Nếu phát hiện cờ TU trong DMASR, hãy xóa cờ và gọi Poll Demand tại đây
        if (eth_handle->eth_dma->DMASR & (1UL << 2)) {
            eth_handle->eth_dma->DMASR = (1UL << 2); // Xóa cờ TU
            eth_handle->eth_dma->DMATPDR = 0;
        }
        eth_handle->tx_state = RUN;
        break;
		break;
/*	case COMPLE:
		//eth_handle->TX_BUFFER->Status|=(1<<31);
		eth_handle->TX_BUFFER->Status |= (1UL << 31) | (1UL << 29) | (1UL << 28);
        // Đánh thức DMA nếu nó đang ở trạng thái Suspend (Bước 9 trong quy trình)
        eth_handle->eth_dma->DMATPDR = 0;
		eth_handle->tx_state=RUN;
		break;*/
	default:
		eth_handle->tx_state=IDLE;
		break;
	}
}
uint8_t ETH_RX_TRANS(ETH_Handle_t*eth_handle,uint8_t ** packet,uint16_t*length)
{
	switch (eth_handle->rx_state)
	{
	case RUN:
			if((eth_handle->RX_BUFFER->Status&(1<<31)))
			{
				//eth_handle->eth_dma->DMAOMR|=1<<24;
				eth_handle->rx_state=SUPSPEND;
			}
			else
			{
				*packet=*(uint32_t*)eth_handle->RX_BUFFER->Buffer1Addr;
				*length=(eth_handle->RX_BUFFER->Status>>16) & 0x3fff;
				eth_handle->rx_state=COMPLE;
			}
		break;
	case SUPSPEND:
		if(eth_handle->eth_dma->DMASR&(1<<7))
		{
			eth_handle->eth_dma->DMASR|=(1<<7);
			eth_handle->eth_dma->DMARPDR = 0;
		}
        eth_handle->rx_state = RUN;
        break;
		break;
	case COMPLE:
		//eth_handle->TX_BUFFER->Status|=(1<<31);
		eth_handle->RX_BUFFER->Status |= (1UL << 31);
        // Đánh thức DMA nếu nó đang ở trạng thái Suspend (Bước 9 trong quy trình)
        eth_handle->eth_dma->DMARPDR = 0;
		eth_handle->rx_state=RUN;
		return 1;
		break;
	default:
		eth_handle->rx_state=IDLE;
		break;
	}
	return 0;
}
extern uint8_t type;

void ETH_TYPE(ETH_Handle_t *eth_handle)
{
	static volatile PACKET_REPLY packet_reply;
	uint16_t eth_type=(uint16_t)eth_handle->RX->MAC_client_length_type[0]<<8|eth_handle->RX->MAC_client_length_type[1];

	switch(eth_type)
	{
	case 0x0806: // =========================================================
	{            // TẦNG 2: XỬ LÝ GÓI ARP (Giữ nguyên logic gốc của bạn)
		         // =========================================================
		uint64_t dst_mac =
		    ((uint64_t)eth_handle->RX->Destination_addres[0] << 40) |
		    ((uint64_t)eth_handle->RX->Destination_addres[1] << 32) |
		    ((uint64_t)eth_handle->RX->Destination_addres[2] << 24) |
		    ((uint64_t)eth_handle->RX->Destination_addres[3] << 16) |
		    ((uint64_t)eth_handle->RX->Destination_addres[4] << 8)  |
		    ((uint64_t)eth_handle->RX->Destination_addres[5]);
		if(dst_mac== 0xFFFFFFFFFFFF||dst_mac==eth_handle->eth_x.mac_addr)
		{
			uint16_t code_eth_arp=(uint16_t)eth_handle->RX->data[6]<<8|eth_handle->RX->data[7];
			if(code_eth_arp==1)
			{
				uint32_t des_ip_addr=	((uint32_t)eth_handle->RX->data[24] << 24) |
					    				((uint32_t)eth_handle->RX->data[25] << 16) |
										((uint32_t)eth_handle->RX->data[26] << 8)  |
										((uint32_t)eth_handle->RX->data[27]);
				if(des_ip_addr==eth_handle->eth_x.ip_addr)
				{
					type=1;
					packet_reply.packet_type=REPLY_TYPE_ARP;
					ETH_PACKET_REPLY(eth_handle,eth_handle->eth_x.mac_addr,eth_handle->eth_x.ip_addr,&packet_reply,60);
					eth_handle->RX->MAC_client_length_type[0]=0;
					eth_handle->RX->MAC_client_length_type[1]=0;
				}
			}
			else if(code_eth_arp==2)
			{
			}
		}
		break;
	}

	case 0x0800: // =========================================================
	{            // TẦNG 3: ĐỊNH TUYẾN IP
		         // =========================================================
			uint8_t ip_protocol=eth_handle->RX->data[9];
			uint8_t *ip = eth_handle->RX->data;

			// Kiểm tra xem gói tin IP này có đúng là gửi cho STM32 không
			uint32_t des_ip_addr =
					((uint32_t)ip[16] << 24) |
					((uint32_t)ip[17] << 16) |
					((uint32_t)ip[18] << 8)  |
					((uint32_t)ip[19]);

			uint16_t ip_total_len = ((uint16_t)ip[2] << 8) | (uint16_t)ip[3];
			switch(ip_protocol)
			{
			case 0x06: // GIAO THỨC TCP -> CHUYỂN GIAO HOÀN TOÀN
			{

			    if(des_ip_addr == eth_handle->eth_x.ip_addr)
			    {
			        type = 2; // Giữ lại cờ nhận diện tầng TCP của bạn nếu cần dùng

			        // Tính tổng chiều dài gói IP nhận được
			        //uint16_t ip_total_len = ((uint16_t)ip[2] << 8) | (uint16_t)ip[3];

			        // --- ĐƯỜNG BIÊN TÁCH TẦNG ---
			        // Gọi tầng TCP xử lý. Toàn bộ đống switch-case máy trạng thái biến mất!
			        TCP_Process_Rx(eth_handle, ip, ip_total_len);
					eth_handle->RX->MAC_client_length_type[0]=0;
					eth_handle->RX->MAC_client_length_type[1]=0;
			    }
			    break;
			}
			case 0x11:
			{
			    if(des_ip_addr == eth_handle->eth_x.ip_addr)
			    {
			        type = 3; // Giữ lại cờ nhận diện tầng TCP của bạn nếu cần dùng

			        // Tính tổng chiều dài gói IP nhận được


			        // --- ĐƯỜNG BIÊN TÁCH TẦNG ---
			        // Gọi tầng UDP xử lý. Toàn bộ đống switch-case máy trạng thái biến mất!
			        UDP_Process_Rx(eth_handle, ip, ip_total_len);
					eth_handle->RX->MAC_client_length_type[0]=0;
					eth_handle->RX->MAC_client_length_type[1]=0;
			    }
				break;
			}
			default:
				break;
			}
		break;
	}
	default:
			type=0;
		break;
	}
}
void ETH_IRQ_Handle(ETH_Handle_t*eth_handle)
{
	 if(eth_handle->eth_dma->DMASR&(1<<7))
		 {
		 	 eth_handle->rx_state = RUN;

			 eth_handle->eth_dma->DMASR=(1<<7)|1<<15;
			 eth_handle->eth_dma->DMARPDR = 0;


		 }
 	  if(eth_handle->eth_dma->DMASR&(1<<6))
	 {
		  	eth_handle->rx_state=RUN;
			eth_handle->RX=eth_handle->RX_BUFFER->Buffer1Addr;
			eth_handle->rx_length=(eth_handle->RX_BUFFER->Status>>16) & 0x3fff;
			eth_handle->rx_state=COMPLE;

			eth_handle->RX_BUFFER->Status |= (1UL << 31);
			eth_handle->eth_dma->DMARPDR = 0;
			//eth_handle->rx_state=RUN;
			eth_handle->eth_dma->DMASR=(1<<6)|(1<<16);
			//  ETH_TYPE(eth_handle);

	 }
 	  if(eth_handle->eth_dma->DMASR&(1<<14))
 	  {
 		 eth_handle->eth_dma->DMASR=(1<<14)|(1<<16);
 	  }
 	  //ETH_TYPE(eth_handle);

}
