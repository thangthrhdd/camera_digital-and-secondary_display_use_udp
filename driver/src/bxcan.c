/*
 * bxcan.c
 *
 *  Created on: Jun 29, 2026
 *      Author: ADMIN
 */
#include "bxcan.h"
void CAN_CLK(CAN_RegDef_t * canx,uint8_t EnorDi)
{
	if(EnorDi==ENABLE)
	{
		if(canx==CAN1)
		{
			CAN1_PCLK_EN();
		}
		else if(canx==CAN2)
		{
			CAN2_PCLK_EN();
		}
	}
	else
	{
		if(canx==CAN1)
		{
			CAN1_PCLK_DI();
		}
		else if(canx==CAN2)
		{
			CAN2_PCLK_DI();
		}
	}
}
void CAN_IRQ_Init(CAN_Handle_t *can_handle)
{
	can_handle->canx->CAN_Control.IER&=~(0x7f);
	can_handle->canx->CAN_Control.IER|=0x7f;
}
void CAN_Init(CAN_Handle_t *can_handle)
{
	CAN_CLK(can_handle->canx,ENABLE);
	uint8_t mode=can_handle->canx->CAN_Control.MSR&(0x03);
	can_handle->state=mode;
	if(mode==CAN_STATE_SLEEP)
	{
		can_handle->canx->CAN_Control.MCR&=~(1<<1);
		can_handle->canx->CAN_Control.MCR|=(1<<0);
		while((can_handle->canx->CAN_Control.MSR&(0x03))!=0x01);
		can_handle->state=CAN_STATE_INIT;
	}
			can_handle->canx->CAN_Control.MCR&=~(0x07<<2);
			can_handle->canx->CAN_Control.MCR|=(0x03<<2)|(can_handle->cany.auto_retransmission<<4);

			can_handle->canx->CAN_Control.BTR&=~(0x03<<30);
			can_handle->canx->CAN_Control.BTR|=(can_handle->cany.can_mode<<30);
			//time stamp
			can_handle->canx->CAN_Control.BTR&=~(0x3ff);
			can_handle->canx->CAN_Control.BTR&=~(0xFF<<16);
			can_handle->canx->CAN_Control.BTR|=(15<<16|7<<20);
			uint8_t BPR=(36000000/(25*can_handle->cany.baudrate))-1;
			can_handle->canx->CAN_Control.BTR|=BPR;


			//enter normal_state
			can_handle->canx->CAN_Control.MCR&=~(1<<1);
			can_handle->canx->CAN_Control.MCR&=~(1<<0);
			while((can_handle->canx->CAN_Control.MSR&(0x03))!=0x00);
			can_handle->state=CAN_STATE_NORMAL;
}
void CAN_Filter_Init(CAN_Handle_t *can_handle,CAN_Filter_config canz)
{
	//filter
	can_handle->canx->CAN_Filter.FMR|=1;
	can_handle->canx->CAN_Filter.FM1R&=~(1<<canz.channel);
	can_handle->canx->CAN_Filter.FM1R|=canz.Filter_mode<<canz.channel;

	can_handle->canx->CAN_Filter.FS1R&=~(1<<canz.channel);
	can_handle->canx->CAN_Filter.FS1R|=canz.Filter_scale<<canz.channel;

	can_handle->canx->CAN_Filter.FFA1R&=~(1<<canz.channel);
	can_handle->canx->CAN_Filter.FFA1R|=canz.assigment_fifo<<canz.channel;

	can_handle->canx->CAN_Filter.FA1R&=~(1<<canz.channel);
	can_handle->canx->CAN_Filter.FA1R|=canz.active<<canz.channel;

	can_handle->canx->CAN_Filter.FR[canz.channel][0]=canz.filter_id1;
	can_handle->canx->CAN_Filter.FR[canz.channel][1]=canz.filter_id2;

	can_handle->canx->CAN_Filter.FMR&=~1; //deinit
}

static void CAN_TX_Handle(CAN_Handle_t *can_handle,uint16_t id,uint8_t  data[8],uint16_t length,uint8_t mailbox_empty)
{

	//switch(can_handle->tx_state[mailbox_empty])
	{
		//case CAN_TX_STATE_EMPTY:
		{
			can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TIR&=~(0x7ff<<21|1<<1|1<<2);
			can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TIR|=((id&(0x7ff))<<21);

			can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDTR&=~(0x0f);
			can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDTR|=(length&0x0f);

			for(int i=0;i<4;i++)
			{
				can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDLR&=~(0xff<<i*8);
				can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDLR|=((data[i]&0xff)<<i*8);
			}
			for(int i=0;i<4;i++)
			{
				can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDHR&=~(0xff<<i*8);
				can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TDHR|=((data[i+4]&0xff)<<i*8);
			}
			can_handle->canx->CAN_Mailbox.TX[mailbox_empty].TIR|=1;
			can_handle->tx_state[mailbox_empty]=CAN_TX_STATE_PENDING;

		}
		/*//	break;
		//case CAN_TX_STATE_PENDING:
		{
			can_handle->tx_state[mailbox_empty]=CAN_TX_STATE_SCHEDULED;
		}
		//	break;
		//case CAN_TX_STATE_SCHEDULED:
		{
			if(!(can_handle->canx->CAN_Control.TSR&(1<<mailbox_empty*8+1)))
			{
				can_handle->tx_state[mailbox_empty]=CAN_TX_STATE_SCHEDULED;
			}
			else
			can_handle->tx_state[mailbox_empty]=CAN_TX_STATE_TRANSMIT;
		}
		//	break;
		//case CAN_TX_STATE_TRANSMIT:
		{
			can_handle->canx->CAN_Control.TSR|=(1<<mailbox_empty*8);
			can_handle->tx_state[mailbox_empty]=CAN_TX_STATE_EMPTY;
		}
		//	break;*/
	}

}
void CAN_TX_Transmit(CAN_Handle_t *can_handle,uint16_t id,uint8_t  data[8],uint16_t length)
{
	uint8_t mailbox=(can_handle->canx->CAN_Control.TSR>>24)&0x03;
	CAN_TX_Handle(can_handle, id,data,length,mailbox);

}
static void handle_data_rx(CAN_Handle_t *can_handle,uint8_t fifo,uint8_t index)
{
	if(can_handle->canx->CAN_Mailbox.RX[fifo].RIR&(1<<2))
	{
		can_handle->rx_packet[fifo][index].id=(can_handle->canx->CAN_Mailbox.RX[fifo].RIR>>3)&0x3ffff;
	}
	else
	{
		can_handle->rx_packet[fifo][index].id=(can_handle->canx->CAN_Mailbox.RX[fifo].RIR>>21)&0x7ff;
	}
	if(!(can_handle->canx->CAN_Mailbox.RX[fifo].RIR&(1<<1)))//data_Frame
	{
		can_handle->rx_packet[fifo][index].length=can_handle->canx->CAN_Mailbox.RX[fifo].RDTR&0x0f;
		if(can_handle->rx_packet[fifo][index].length<4)
		{
			for(uint8_t i=0;i<can_handle->rx_packet[fifo][index].length;i++)
			{
				can_handle->rx_packet[fifo][index].data[i]=(can_handle->canx->CAN_Mailbox.RX[fifo].RLR>>i*8)&0xff;
			}
		}
		else
		{
			for(uint8_t i=0;i<4;i++)
			{
				can_handle->rx_packet[fifo][index].data[i]=(can_handle->canx->CAN_Mailbox.RX[fifo].RLR>>i*8)&0xff;
			}
			for(uint8_t i=0;i<can_handle->rx_packet[fifo][index].length-4;i++)
			{
				can_handle->rx_packet[fifo][index].data[i+4]=(can_handle->canx->CAN_Mailbox.RX[fifo].RHR>>i*8)&0xff;
			}
		}
	}
}
static void CAN_RX_Handle(CAN_Handle_t *can_handle,uint8_t fifo)
{
	uint8_t valid_msg=0;
	switch(can_handle->rx_state[fifo])
	{
		case CAN_RX_STATE_EMPTY:
		{
			valid_msg=can_handle->canx->CAN_Control.RFR[fifo]&0x03;
			if(valid_msg==1)
			{
				can_handle->rx_state[fifo]=CAN_RX_STATE_PENDING_1;
			}
			else if(valid_msg==2)
			{
				can_handle->rx_state[fifo]=CAN_RX_STATE_PENDING_2;
			}
			else if(valid_msg==3)
			{
				can_handle->rx_state[fifo]=CAN_RX_STATE_PENDING_3;
			}
		}
		 break;
		case CAN_RX_STATE_PENDING_1:
		{

			handle_data_rx(can_handle,fifo,0);
			can_handle->canx->CAN_Control.RFR[fifo]|=1<<5;
			valid_msg=can_handle->canx->CAN_Control.RFR[fifo]&0x03;
			if(valid_msg>0)
			{
				can_handle->rx_state[fifo]=valid_msg;
			}
			else
			can_handle->rx_state[fifo]=CAN_RX_STATE_EMPTY;

		}
		 break;
		case CAN_RX_STATE_PENDING_2:
		{
			handle_data_rx(can_handle,fifo,1);
			can_handle->canx->CAN_Control.RFR[fifo]|=1<<5;
			valid_msg=can_handle->canx->CAN_Control.RFR[fifo]&0x03;
			if(valid_msg>0)
			{
				can_handle->rx_state[fifo]=valid_msg;
			}
			else
			can_handle->rx_state[fifo]=CAN_RX_STATE_PENDING_1;
		}
		 break;
		case CAN_RX_STATE_PENDING_3:
		{
			handle_data_rx(can_handle,fifo,2);
			can_handle->canx->CAN_Control.RFR[fifo]|=1<<5;
			valid_msg=can_handle->canx->CAN_Control.RFR[fifo]&0x03;
			if(valid_msg>0)
			{
				can_handle->rx_state[fifo]=valid_msg;
			}
			else
			can_handle->rx_state[fifo]=CAN_RX_STATE_PENDING_2;
		}
		 break;
		case CAN_RX_STATE_OVERRUN:
		{

		}
		break;
	}
}
void CAN_RX_Transmit(CAN_Handle_t *can_handle,uint8_t fifo)
{
	CAN_RX_Handle(can_handle, fifo);
	while(can_handle->rx_state[fifo]!=CAN_RX_STATE_EMPTY)
	{
		CAN_RX_Handle(can_handle, fifo);
	}

}
void CAN_IRQ_TX_Handle(CAN_Handle_t* can_handle)
{
	if(can_handle->canx->CAN_Control.TSR&0x01)
	{
		if(can_handle->canx->CAN_Control.TSR&(1<<1)) //transmit ok
		{
			can_handle->tx_state[0]=CAN_TX_STATE_EMPTY;
		}
		else
		{
			can_handle->tx_state[0]=CAN_TX_STATE_ERROR;
		}
		can_handle->canx->CAN_Control.TSR|=1;
	}
	if(can_handle->canx->CAN_Control.TSR&(1<<8))
	{
		if(can_handle->canx->CAN_Control.TSR&(1<<9)) //transmit ok
		{
			can_handle->tx_state[1]=CAN_TX_STATE_EMPTY;
		}
		else
		{
			can_handle->tx_state[1]=CAN_TX_STATE_ERROR;
		}
		can_handle->canx->CAN_Control.TSR|=(1<<8);
	}
	if(can_handle->canx->CAN_Control.TSR&(1<<16))
	{
		if(can_handle->canx->CAN_Control.TSR&(1<<17)) //transmit ok
		{
			can_handle->tx_state[2]=CAN_TX_STATE_EMPTY;
		}
		else
		{
			can_handle->tx_state[2]=CAN_TX_STATE_ERROR;
		}
		can_handle->canx->CAN_Control.TSR|=(1<<16);
	}

}
void CAN_IRQ_RXFF0_Handle(CAN_Handle_t* can_handle)
{
	if((can_handle->canx->CAN_Control.RFR[0]&0x03)>0)
	{
		uint8_t valid_msg=can_handle->canx->CAN_Control.RFR[0]&0x03;
		can_handle->rx_state[0]=valid_msg;
		CAN_RX_Handle(can_handle,0);
	}
	else if(can_handle->canx->CAN_Control.RFR[0]&(1<<3))
	{
		can_handle->canx->CAN_Control.RFR[0]|=1<<3;
	}
	else if(can_handle->canx->CAN_Control.RFR[0]&(1<<4))
	{
		can_handle->canx->CAN_Control.RFR[0]|=1<<4;
	}
}
void CAN_IRQ_RXFF1_Handle(CAN_Handle_t* can_handle)
{
	if((can_handle->canx->CAN_Control.RFR[1]&0x03)>0)
	{
		uint8_t valid_msg=can_handle->canx->CAN_Control.RFR[1]&0x03;
		can_handle->rx_state[1]=valid_msg;
		CAN_RX_Handle(can_handle,1);
	}
	else if(can_handle->canx->CAN_Control.RFR[1]&(1<<3))
	{
		can_handle->canx->CAN_Control.RFR[1]|=1<<3;
	}
	else if(can_handle->canx->CAN_Control.RFR[1]&(1<<4))
	{
		can_handle->canx->CAN_Control.RFR[1]|=1<<4;
	}
}
