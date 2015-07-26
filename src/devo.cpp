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

DEVO::DEVO(Output&o) :
		CYRF(GPIO_Pin(GPIOA, GPIO_Pin_4), GPIO_Pin(GPIOB, GPIO_Pin_0), SPI1), output(o)
{
	channel_packets = 0xff;
	fixed_id = 0;
	transmitter_id = 0;
	chns[0] = chns[1] = chns[2] = 0;
	chns_idx = 0;

	bind_packets = 0;
	use_fixed_id = false;
	RFStatus = Uninitialized;
	last_packet_tick = 0;
	wait_tick = 0;
	missed_packet = 0;
}

void DEVO::Init()
{
	SEGGER_RTT_WriteString(0, "---- begin DEVO_Initialize ----\n");
	CLOCK_StopTimer();
	CYRF.CS_HI();
	CYRF.Reset();
	CYRF.WriteRegister(TX_LENGTH_ADR, 0x55);
	u8 r = CYRF.ReadRegister(TX_LENGTH_ADR);
	if (r != 0x55)
	{
		SEGGER_RTT_WriteString(0, "cyrf6936 fail!!\n");
		SetLED(CYRF_Fail);
		return;
	}
	CYRF.Init();
	CYRF.ConfigCRCSeed(0x0);
	CYRF.ConfigSOPCode(sopcodes[0]);
	CYRF.ConfigRxTx(0);

	CYRF.ConfigRFChannel(0x4);
	RFStatus = Initialized; //初始化完成 开始等待发射机绑定
	StartReceive();
	wait_tick=Get_Ticks();
	missed_packet=0;
	CLOCK_StartTimer(200, DEVO_Callback);
}

u16 cal_channel_value(u16 raw, u8 sig)
{
	u16 chn_val;
	chn_val = raw;
	chn_val = chn_val > 1600 ? 1600 : chn_val;
	if (sig)
	{
		chn_val = 1600 - chn_val;
	}
	else
	{
		chn_val += 1600;
	}
	chn_val += (chn_val << 2);
	chn_val >>= 3;
	return chn_val;
}
//return true when this packet is processed!
// otherwise this packet has been ignored.
bool DEVO::ProcessPacket(u8 pac[])
{
	bool retval = false;
	int idx = 0;
	u8 RFChannel = CYRF.GetRFChannel();
	switch (pac[0] & 0xf)
	{
	case 0xa: // Bind包: 收到这个包开始bind过程
		if (RFStatus != Initialized && RFStatus != Binding)
		{
			SEGGER_RTT_WriteString(0,
					"Bind error: RFStatus != Initialized && RFStatus != Binding\n");
			break;
		}
		//保存transmit channels
		chns[0] = chns[3] = pac[3];
		chns[1] = chns[4] = pac[4];
		chns[2] = pac[5];
		SEGGER_RTT_printf(0, "tx chn=%x %x %x\n", chns[0], chns[1], chns[2]);

		//当前Channel必须在transmit channels中
		if (RFChannel == chns[0])
			chns_idx = 0;
		else if (RFChannel == chns[1])
			chns_idx = 1;
		else if (RFChannel == chns[2])
			chns_idx = 2;
		else
		{
			SEGGER_RTT_printf(0,
					"Bind error: RFChannel=%x ch1=%x ch2=%x ch3=%x\n",
					RFChannel, chns[0], chns[1], chns[2]);
			break;
		}
		//接下的两个频道也必须符合
		if (chns[chns_idx + 1] != pac[11] || chns[chns_idx + 2] != pac[12])
		{
			SEGGER_RTT_WriteString(0, "Bind error: next channel not match 1\n");
			break;
		}
		//发射机id
		transmitter_id = *((u32*) (&pac[6]));
		//剩余绑定包
		bind_packets = *((u16*) (&pac[1]));

		channel_packets = pac[10] & 0xf;

		//TODO: fixed_id机制要完善
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
		//if (RFStatus != Binding)
			SEGGER_RTT_printf(0,
					"Binding! bind_packets=%d, channel_packets=%d\n",
					bind_packets, channel_packets);
		RFStatus = Binding;
		SetLED(RF_Bound);
		retval = true;
		break;
	case 0xd: //数据包
	case 0xc:
	case 0xb:
		if (RFStatus != Binding && RFStatus != Bound) //因为这个必须用transmitter_id解密
		{
			SEGGER_RTT_printf(0, "Data error: RFStatus=%d(%d %d)\n", RFStatus,
					Binding, Bound);
			break;
		}
		if (bind_packets)
			bind_packets--;
		idx = (pac[0] & 0xf) - 0xb;
		idx <<= 2;
		ScramblePacket(pac); //解密包
		//解析4个通道
		Channels[idx + 0] = cal_channel_value(*((u16*) (&pac[1])),
				pac[9] & 0x80);
		output.SetChannelValue(idx + 0, Channels[idx + 0]);
		Channels[idx + 1] = cal_channel_value(*((u16*) (&pac[3])),
				pac[9] & 0x40);
		output.SetChannelValue(idx + 1, Channels[idx + 1]);
		Channels[idx + 2] = cal_channel_value(*((u16*) (&pac[5])),
				pac[9] & 0x20);
		output.SetChannelValue(idx + 2, Channels[idx + 2]);
		Channels[idx + 3] = cal_channel_value(*((u16*) (&pac[7])),
				pac[9] & 0x10);
		output.SetChannelValue(idx + 3, Channels[idx + 3]);

		SEGGER_RTT_printf(0,
				"Channels: %05d %05d %05d %05d %05d %05d %05d %05d\n",
				Channels[0], Channels[1], Channels[2], Channels[3], Channels[4],
				Channels[5], Channels[6], Channels[7]);
		switch (pac[10] & 0xf0)
		{
		case 0xc0:
		case 0x80:
			use_fixed_id = true;
			//固定id
			fixed_id = (*((u32*) (&pac[13]))) & 0xffffff;
			break;
		case 0x00:
			use_fixed_id = false;
			fixed_id = 0;
			break;
		}
		channel_packets = pac[10] & 0xf;
		if (chns[chns_idx + 1] != pac[11] || chns[chns_idx + 2] != pac[12])
		{
			SEGGER_RTT_WriteString(0, "Error: next channel not match 2\n");
			break;
		}
		retval = true;
		break;
	case 0x7: //fail-safe info for 1st 8 data-channels
	case 0x8: //               for the upper 8 channels
		if (RFStatus != Bound) //应该只能在绑定结束后收到
		{
			SEGGER_RTT_printf(0,
					"fail-safe error: RFStatus != Bound  RFStatus=%d\n",RFStatus);
			break;
		}
		ScramblePacket(pac); //解密包
		SEGGER_RTT_WriteString(0, "fail-safe info\n");
		switch (pac[10] & 0xf0)
		{
		case 0xc0:
		case 0x80:
			use_fixed_id = true;
			//固定id
			fixed_id = (*((u32*) (&pac[13]))) & 0xffffff;
			break;
		case 0x00:
			use_fixed_id = false;
			fixed_id = 0;
			break;
		}
		channel_packets = pac[10] & 0xf;
		if (chns[chns_idx + 1] != pac[11] || chns[chns_idx + 2] != pac[12])
		{
			SEGGER_RTT_WriteString(0, "Error: next channel not match 3\n");
			break;
		}
		retval = true;
		break;
	default:
		SEGGER_RTT_printf(0,
				"packet: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				pac[0], pac[1], pac[2], pac[3], pac[4], pac[5], pac[6], pac[7],
				pac[8], pac[9], pac[10], pac[11], pac[12], pac[13], pac[14],
				pac[15]);
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
	u32 now_tick=Get_Ticks();
	bool need_reset_rx = false;
	u16 retval=0;
	u8 RFChannel = CYRF.GetRFChannel();
	u8 RX_IRQ_STATUS = CYRF.ReadRegister(RX_IRQ_STATUS_ADR); //读RX_IRQ
	if ((RX_IRQ_STATUS & (RXC_IRQ | RXE_IRQ | RXBERR_IRQ)) == RXC_IRQ) // 一个包被接收
	{
		if (RX_IRQ_STATUS & RXOW_IRQ)
		{
			CYRF.WriteRegister(RX_IRQ_STATUS_ADR, RXOW_IRQ);
			SEGGER_RTT_WriteString(0, "Overwriten!!\n");
		}
		u8 RX_STATUS = CYRF.ReadRegister(RX_STATUS_ADR); //读RX状态
		u8 pac[20];
		for (int i = 0; i < 5; i++)
			*((u32*) (&pac[i << 2])) = 0;
		CYRF.ReadDataPacket(pac); //读包
		retval=400;
		//SEGGER_RTT_printf(0, "*%02X@%02X#%02X!%02X\n", pac[0], RFChannel,
		//		RX_IRQ_STATUS, RX_STATUS);
		//SEGGER_RTT_printf(0, "pac@%x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",CYRF.GetRFChannel(),
		//		pac[0],pac[1],pac[2],pac[3],pac[4],pac[5],pac[6],pac[7],pac[8],pac[9],pac[10],pac[11],pac[12],pac[13],pac[14],pac[15]);
		need_reset_rx = true;
		// 处理包
		if (ProcessPacket(pac)) //合适的包
		{
			SEGGER_RTT_printf(0, ".%02x#%x(%d)\n",RX_IRQ_STATUS,pac[0]&0xf,now_tick-last_packet_tick);
			wait_tick=last_packet_tick=now_tick;
			//SEGGER_RTT_printf(0, "good packet\n");
			//SEGGER_RTT_printf(0, "[%d,%d,%d]\n", bind_packets, channel_packets,
			//		Get_Ticks());
			if (RFStatus==Binding && bind_packets == 0)
			{
				//SEGGER_RTT_printf(0,
				//		"packet: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
				//		pac[0], pac[1], pac[2], pac[3], pac[4], pac[5], pac[6],
				//		pac[7], pac[8], pac[9], pac[10], pac[11], pac[12],
				//		pac[13], pac[14], pac[15]);
				SEGGER_RTT_printf(0, "[%d,%d]\n", bind_packets, channel_packets);
				SetBoundSOPCode();
				RFStatus = Bound;
			}
			if (channel_packets == 0)
			{
				chns_idx = (chns_idx < 2) ? (chns_idx + 1) : 0;
				channel_packets = 4;
//				SEGGER_RTT_printf(0, "\t\t\t\t\t\tchannel change %02X\n",
//						chns[chns_idx]);
				CYRF.ConfigRFChannel(chns[chns_idx]);
			}
			StartReceive();
			return retval;
		}
		else
		{
			SEGGER_RTT_printf(0, "bad");
		}
	}
	else if(RX_IRQ_STATUS & RXC_IRQ) // RXC_IRQ被置位 且 RXE_IRQ或RXBERR_IRQ被置位
	{
		StartReceive();
		retval=200;
	}
	else
	{
		retval=200;
	}
	//没有包到达 或包不是bind包
	if (RX_IRQ_STATUS)
		SEGGER_RTT_printf(0, ".%02x\n", RX_IRQ_STATUS);
	if ((RFStatus == Initialized) && ((now_tick-wait_tick) >= 2800)) // 等待超过2.8ms
	{
		RFChannel = (RFChannel > 0x4f) ? 0x4 : (RFChannel + 1); //循环0x4-0x4F频道
		CYRF.ConfigRFChannel(RFChannel);
		need_reset_rx = true;
		wait_tick = now_tick;
	}
	else if (((RFStatus == Bound) || (RFStatus == Binding)))
	{
		if (((now_tick-wait_tick) > channel_packets * 2400))
		{
			//测算为接受的包数大于等于剩余包
			missed_packet += channel_packets;
			if (missed_packet > 100) //错过100个包认为失去信号
			{
				SEGGER_RTT_WriteString(0, "ERROR: I think i lost the signal\n");
				RFStatus = Lost; //Tx包应该2.4ms一次
				SetLED(RF_Lost);
				return 0;
			}
			else
			{
				SEGGER_RTT_printf(0, "here i lost %d packets(tick=%d)\n",channel_packets,now_tick-wait_tick);
				channel_packets = 4; //每个频道4个包
				chns_idx = (chns_idx < 2) ? (chns_idx + 1) : 0;
				CYRF.ConfigRFChannel(chns[chns_idx]);
				retval=300;
			}
		}
	}
	if (need_reset_rx)
	{
		StartReceive();
	}
	return retval;

}

void DEVO::SetBoundSOPCode(void)
{
	/* crc == 0 isn't allowed, so use 1 if the math results in 0 */
	u8*cyrfmfg_id = (u8*) &transmitter_id;
	u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
	if (!crc)
		crc = 1;
	u8 sopidx = (0xff & ((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2]))
			% 10;
	CYRF.ConfigCRCSeed((crc << 8) + crc);
	CYRF.ConfigSOPCode(sopcodes[sopidx]);
}

