#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include <stdio.h>
#include "main.h"
#include "delay.h"
#include "devo.h"
#include "cyrf.h"
#include "oled.h"
#include "timx.h"

#define PKTS_PER_CHANNEL 4
#define NUM_WAIT_LOOPS (100 / 5)
#define CHAN_MULTIPLIER 100
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define NUM_OUT_CHANNELS 12
#define BIND_COUNT 0x1388
enum PktState
{
	DEVO_BIND = 1,
	DEVO_BIND_SENDCH = 2,
	DEVO_BOUND = 3,
	DEVO_BOUND_1 = 4,
	DEVO_BOUND_2 = 5,
	DEVO_BOUND_3 = 6,
	DEVO_BOUND_4 = 7,
	DEVO_BOUND_5 = 8,
	DEVO_BOUND_6 = 9,
	DEVO_BOUND_7 = 10,
	DEVO_BOUND_8 = 11,
	DEVO_BOUND_9 = 12,
	DEVO_BOUND_10 = 13,
};
u8 num_channels;
u8 packet[16];
int bind_counter;
u8 use_fixed_id;
u8 pkt_num;
unsigned long fixed_id;
u8 txState;
enum PktState state;
u8 ch_idx;
int Channels[NUM_OUT_CHANNELS];
u8 failsafe_pkt;
u8 radio_ch[5];
u8 *radio_ch_ptr;
u8 cyrfmfg_id[6];
CYRF6936 CYRF(GPIOA,GPIO_Pin_3,GPIO_Pin_4,SPI1);

void build_bind_pkt()
{
	packet[0] = (num_channels << 4) | 0x0a;
	packet[1] = bind_counter & 0xff;
	packet[2] = (bind_counter >> 8);
	packet[3] = *radio_ch_ptr;
	packet[4] = *(radio_ch_ptr + 1);
	packet[5] = *(radio_ch_ptr + 2);
	packet[6] = cyrfmfg_id[0];
	packet[7] = cyrfmfg_id[1];
	packet[8] = cyrfmfg_id[2];
	packet[9] = cyrfmfg_id[3];
	add_pkt_suffix();
	//The fixed-id portion is scrambled in the bind packet
	//I assume it is ignored
	packet[13] ^= cyrfmfg_id[0];
	packet[14] ^= cyrfmfg_id[1];
	packet[15] ^= cyrfmfg_id[2];
}

void add_pkt_suffix()
{
	u8 bind_state;
	if (use_fixed_id)
	{
		if (bind_counter > 0)
		{
			bind_state = 0xc0;
		}
		else
		{
			bind_state = 0x80;
		}
	}
	else
	{
		bind_state = 0x00;
	}
	packet[10] = bind_state | (PKTS_PER_CHANNEL - pkt_num - 1);
	packet[11] = *(radio_ch_ptr + 1);
	packet[12] = *(radio_ch_ptr + 2);
	packet[13] = fixed_id & 0xff;
	packet[14] = (fixed_id >> 8) & 0xff;
	packet[15] = (fixed_id >> 16) & 0xff;
}

static void build_data_pkt()
{
	u8 i;
	packet[0] = (num_channels << 4) | (0x0b + ch_idx);
	u8 sign = 0x0b;
	for (i = 0; i < 4; i++)
	{
		long value = (long) Channels[ch_idx * 4 + i] * 0x640 / CHAN_MAX_VALUE;
		if (value < 0)
		{
			value = -value;
			sign |= 1 << (7 - i);
		}
		packet[2 * i + 1] = value & 0xff;
		packet[2 * i + 2] = (value >> 8) & 0xff;
	}
	packet[9] = sign;
	ch_idx = ch_idx + 1;
	if (ch_idx * 4 >= num_channels)
		ch_idx = 0;
	add_pkt_suffix();
}

void cyrf_set_bound_sop_code()
{
	/* crc == 0 isn't allowed, so use 1 if the math results in 0 */
	u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
	if (!crc)
		crc = 1;
	u8 sopidx = (0xff & ((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2]))
			% 10;
	CYRF.ConfigRxTx(1);
	CYRF.ConfigCRCSeed((crc << 8) + crc);
	CYRF.ConfigSOPCode(CYRF6936::sopcodes[sopidx]);
	CYRF.WriteRegister(CYRF_03_TX_CFG, 0x08 | /*Model.tx_power*/7);
}

void scramble_pkt()
{
#ifdef NO_SCRAMBLE
	return;
#else
	u8 i;
	for (i = 0; i < 15; i++)
	{
		packet[i + 1] ^= cyrfmfg_id[i % 4];
	}
#endif
}

void build_beacon_pkt(int upper)
{
	packet[0] = ((num_channels << 4) | 0x07);
	u8 enable = 0;
	int max = 8;
	int offset = 0;
	if (upper)
	{
		packet[0] += 1;
		max = 4;
		offset = 8;
	}
	for (int i = 0; i < max; i++)
	{
		//if (i + offset < Model.num_channels && Model.limits[i+offset].flags & CH_FAILSAFE_EN) {
		//    enable |= 0x80 >> i;
		//    packet[i+1] = Model.limits[i+offset].failsafe;
		//} else {
		packet[i + 1] = 0;
		//}
	}
	packet[9] = enable;
	add_pkt_suffix();
}

void set_radio_channels()
{
	int i;
	CYRF.FindBestChannels(radio_ch, 3, 4, 4, 80);
    radio_ch[3] = radio_ch[0];
    radio_ch[4] = radio_ch[1];
}

void DEVO_Initialize()
{
	CLOCK_StopTimer();
	char buf[32];
	CYRF.CS_HI();
	CYRF.Reset();
	CYRF.Init();
	CYRF.GetMfgData(cyrfmfg_id);
	sprintf(buf,"MFG=%02X %02X %02X %02X %02X %02X",
			cyrfmfg_id[0],cyrfmfg_id[1],cyrfmfg_id[2],cyrfmfg_id[3],cyrfmfg_id[4],cyrfmfg_id[5]);
	oled->print_6x8Str(0,0,buf);

	CYRF.ConfigRxTx(true);
	CYRF.ConfigCRCSeed(0x0000);
	CYRF.ConfigSOPCode(CYRF6936::sopcodes[0]);
	set_radio_channels();

	sprintf(buf,"CHN=%d %d %d",radio_ch[0],radio_ch[1],radio_ch[2]);
	oled->print_6x8Str(0,1,buf);

	use_fixed_id = 0;
	failsafe_pkt = 0;
	radio_ch_ptr = radio_ch;
	CYRF.ConfigRFChannel(*radio_ch_ptr);
	//num_channels = ((Model.num_channels + 3) >> 2) * 4;
	num_channels = (8 + 3)& ~0xfc; //8ͨ通道
	pkt_num = 0;
	ch_idx = 0;
	txState = 0;

	//伪随机
	fixed_id = ((u32) (radio_ch[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
			| ((u32) (radio_ch[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
			| ((u32) (radio_ch[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
	//取模
	fixed_id = fixed_id % 1000000;
	bind_counter = BIND_COUNT;
	state = DEVO_BIND;
	//PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs

	CLOCK_StartTimer(2400, DEVO_Callback);

}

u16 DEVO_Callback()
{
	if (txState == 0)
	{
		//发送数据包
		txState = 1;
		DEVO_BuildPacket();
		CYRF.WriteDataPacket(packet);
		return 1200;
	}
	txState = 0;
	int i = 0;
	//检查发送状态
	u8 IRQ_STATUS = 0;
	while (!((IRQ_STATUS = CYRF.ReadRegister(CYRF_04_TX_IRQ_STATUS)) & 0x02))
	{
        if(++i > NUM_WAIT_LOOPS)
            return 1200;//如果发送失败1200us后重新发送
	}
	if (state == DEVO_BOUND)//已经绑定成功
	{
		/* exit binding state */
		state = DEVO_BOUND_3;
		cyrf_set_bound_sop_code();
	}
	if (pkt_num == 0)
	{
		//Keep tx power updated
		CYRF.WriteRegister(CYRF_03_TX_CFG, 0x08 | /*Model.tx_power*/7);
		radio_ch_ptr = ((radio_ch_ptr == &radio_ch[2]) ? radio_ch : radio_ch_ptr + 1);
		CYRF.ConfigRFChannel(*radio_ch_ptr);
	}
	return 1200;
}

void DEVO_BuildPacket()
{
	switch (state)
	{
	case DEVO_BIND:
		//Serial.println("DEVO_BuildPacket: DEVO_BIND");
		bind_counter--;
		build_bind_pkt();
		state = DEVO_BIND_SENDCH;
		break;
	case DEVO_BIND_SENDCH:
		//Serial.println("DEVO_BuildPacket: DEVO_BIND_SENDCH");
		bind_counter--;
		build_data_pkt();
		scramble_pkt();
		if (bind_counter <= 0)
		{
			state = DEVO_BOUND;
			//TOTO:PROTOCOL_SetBindState(0);
		}
		else
		{
			state = DEVO_BIND;
		}
		break;
	case DEVO_BOUND:
	case DEVO_BOUND_1:
	case DEVO_BOUND_2:
	case DEVO_BOUND_3:
	case DEVO_BOUND_4:
	case DEVO_BOUND_5:
	case DEVO_BOUND_6:
	case DEVO_BOUND_7:
	case DEVO_BOUND_8:
	case DEVO_BOUND_9:
		//Serial.println("DEVO_BuildPacket: DEVO_BOUND");
		build_data_pkt();
		scramble_pkt();
		//state++;
		if (bind_counter > 0)
		{
			bind_counter--;
			//if (bind_counter == 0)
			//TOTO:PROTOCOL_SetBindState(0);
		}
		break;
	case DEVO_BOUND_10:
		//Serial.println("DEVO_BuildPacket: DEVO_BOUND_10");
		build_beacon_pkt(num_channels > 8 ? failsafe_pkt : 0);
		failsafe_pkt = failsafe_pkt ? 0 : 1;
		scramble_pkt();
		state = DEVO_BOUND_1;
		break;
	}
	pkt_num++;
	if (pkt_num == PKTS_PER_CHANNEL)
		pkt_num = 0;
}
