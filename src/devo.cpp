#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include <stdio.h>
#include "main.h"
#include "delay.h"
#include "cyrf.h"
#include "devo.h"
#include "timx.h"
#include "segger_rtt.h"

DEVO* pDEVO;

static const u8 sopcodes[][8] = {
    /* Note these are in order transmitted (LSB 1st) */
    /* 0 */ {0x3C,0x37,0xCC,0x91,0xE2,0xF8,0xCC,0x91}, //0x91CCF8E291CC373C
    /* 1 */ {0x9B,0xC5,0xA1,0x0F,0xAD,0x39,0xA2,0x0F}, //0x0FA239AD0FA1C59B
    /* 2 */ {0xEF,0x64,0xB0,0x2A,0xD2,0x8F,0xB1,0x2A}, //0x2AB18FD22AB064EF
    /* 3 */ {0x66,0xCD,0x7C,0x50,0xDD,0x26,0x7C,0x50}, //0x507C26DD507CCD66
    /* 4 */ {0x5C,0xE1,0xF6,0x44,0xAD,0x16,0xF6,0x44}, //0x44F616AD44F6E15C
    /* 5 */ {0x5A,0xCC,0xAE,0x46,0xB6,0x31,0xAE,0x46}, //0x46AE31B646AECC5A
    /* 6 */ {0xA1,0x78,0xDC,0x3C,0x9E,0x82,0xDC,0x3C}, //0x3CDC829E3CDC78A1
    /* 7 */ {0xB9,0x8E,0x19,0x74,0x6F,0x65,0x18,0x74}, //0x7418656F74198EB9
    /* 8 */ {0xDF,0xB1,0xC0,0x49,0x62,0xDF,0xC1,0x49}, //0x49C1DF6249C0B1DF
    /* 9 */ {0x97,0xE5,0x14,0x72,0x7F,0x1A,0x14,0x72}, //0x72141A7F7214E597
};

DEVO::DEVO()
	: CYRF(GPIO_Pin(GPIOA,GPIO_Pin_4),GPIO_Pin(GPIOB,GPIO_Pin_0),SPI1)
{
}


void DEVO::Init()
{
	SEGGER_RTT_printf(0,"---- begin DEVO_Initialize ----\n");
	CLOCK_StopTimer();
	//CYRF=new CYRF6936(GPIO_Pin(GPIOA,GPIO_Pin_4),GPIO_Pin(GPIOB,GPIO_Pin_0),SPI1);
	CYRF.CS_HI();
	CYRF.Reset();
	CYRF.WriteRegister(TX_LENGTH_ADR,0x2A);
	u8 r=CYRF.ReadRegister(TX_LENGTH_ADR);
	if(r!=0x2A)
	{
		SEGGER_RTT_printf(0,"cyrf6936 fail!!\n");
		SetLED(CYRF_Fail);
		while(true);
	}
	CYRF.Init();
	CYRF.ConfigCRCSeed(0x0);
	CYRF.ConfigSOPCode(sopcodes[0]);
	CYRF.ConfigRxTx(0);

	RFChannel=0x4;
	ChannelRetry=0;
	CYRF.ConfigRFChannel(RFChannel);
	CYRF.WriteRegister(RX_ABORT_ADR,RX_ABORT_RST);
	CYRF.WriteRegister(RX_CTRL_ADR,RX_CTRL_RST|RX_GO);
	RFStatus=Binding;

	CLOCK_StartTimer(200, DEVO_Callback);
}

bool DEVO::ProcessPacket(u8 pac[])
{
	bool retval=false;
	switch (pac[0] & 0xf)
	{
	case 0xa: // Bind包
		//保存transmit channels
		chns[0] = pac[3];
		chns[1] = pac[4];
		chns[2] = pac[5];
		//当前Channel必须在transmit channels中
		if (RFChannel == chns[0])
			chns_idx = 0;
		else if (RFChannel == chns[1])
			chns_idx = 1;
		else if (RFChannel == chns[2])
			chns_idx = 2;
		else
			break;
		//接下的两个频道也必须符合
		if (chns[chns_idx+1] != pac[11] || chns[chns_idx+2] != pac[12])
			break;
		//发射机id
		transmitter_id = *((u32*) (&pac[6]));
		//剩余绑定包
		bind_packets = *((u16*) (&pac[1]));

		channel_packets = pac[10] & 0xf;

		switch (pac[10] & 0xf0)
		{
		case 0xc0:
		case 0x80:
			use_fixed_id = true;
			//固定id
			fixed_id = ((*((u32*) (&pac[13]))) ^ transmitter_id) & 0xffffff;
			break;
		case 0x00:
			use_fixed_id = false;
			fixed_id = 0;
			break;
		}
		RFStatus = Bound;
		SetLED(RF_Bound);
		retval=true;
		break;
	case 0xb:
	case 0xc: //数据包
		if(RFStatus != Bound)
			break;
		scramble_pkt(pac);
		fixed_id = ((*((u32*) (&pac[13]))) ^ transmitter_id) & 0xffffff;
		switch (pac[10] & 0xf0)
		{
		case 0x80:
		case 0xc0:
			use_fixed_id = true;
			break;
		case 0x00:
			use_fixed_id = false;
			break;
		default:
			while (1)
				;
		}
		channel_packets = pac[10] & 0xf;
		if (chns[chns_idx+1] != pac[11] || chns[chns_idx+2] != pac[12])
					while (1)
						;
		retval=true;
		break;
	}

	return retval;
}

u16 DEVO_Callback()
{
	return pDEVO->Callback();
}

u16 DEVO::Callback()
{
	bool need_reset_rx=false;
	u8 RX_IRQ_STATUS=CYRF.ReadRegister(RX_IRQ_STATUS_ADR); //读RX_IRQ
	if((RX_IRQ_STATUS&(RXC_IRQ|RXE_IRQ))==RXC_IRQ) // 一个包被接收
	{
		u8 pac[20];
		CYRF.ReadDataPacket(pac); //读包
		need_reset_rx=true;
		u8 RX_STATUS = CYRF.ReadRegister(RX_STATUS_ADR); //读RX状态
		SEGGER_RTT_printf(0,"%s\n",pac);
		// 处理包
		if(ProcessPacket(pac))//合适的包
		{
			ChannelRetry=0;
			if(channel_packets==0)
			{
				chns_idx=chns_idx<2?chns_idx+1:0;
				CYRF.ConfigRFChannel(chns[chns_idx]);
			}
			CYRF.WriteRegister(RX_ABORT_ADR, RX_ABORT_RST);
			CYRF.WriteRegister(RX_CTRL_ADR, RX_CTRL_RST | RX_GO);
			return 200;
		}
	}
	//没有包到达 或包不是bind包
	ChannelRetry++;
	if (ChannelRetry >= 13) // 每个频道尝试13次 约2.6ms
	{
		ChannelRetry = 0;
		switch (RFStatus)
		{
		case Bound:
			RFStatus = Lost; //Tx包应该2.4ms一次
			SetLED(RF_Lost);
			while (1)
				;
			break;
		case Binding:
			RFChannel = RFChannel >= 0x4f ? 0x4 : RFChannel + 1; //循环0x4-0x4F频道
			CYRF.ConfigRFChannel(RFChannel);
			need_reset_rx=true;
			break;
		}

	}
	if (need_reset_rx)
	{
		CYRF.WriteRegister(RX_ABORT_ADR, RX_ABORT_RST);
		CYRF.WriteRegister(RX_CTRL_ADR, RX_CTRL_RST | RX_GO);
	}
	return 200;

}

