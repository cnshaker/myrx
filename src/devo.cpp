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

#define PKTS_PER_CHANNEL 4
#define NUM_WAIT_LOOPS (100 / 5)
enum PktState {
    DEVO_BIND=0,
    DEVO_BIND_SENDCH=1,
    DEVO_BOUND=2,
    DEVO_BOUND_1=3,
    DEVO_BOUND_2=4,
    DEVO_BOUND_3=5,
    DEVO_BOUND_4=6,
    DEVO_BOUND_5=7,
    DEVO_BOUND_6=8,
    DEVO_BOUND_7=9,
    DEVO_BOUND_8=10,
    DEVO_BOUND_9=11,
    DEVO_BOUND_10=12,
};
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

static s16 bind_counter;
static enum PktState state;
static u8 txState;
static u8 packet[16];
static u32 fixed_id;
static u8 radio_ch[5];
static u8 *radio_ch_ptr;
static u8 pkt_num;
static u8 cyrfmfg_id[6];
static u8 num_channels;
static u8 ch_idx;
static u8 use_fixed_id;
static u8 failsafe_pkt;

#define CHAN_MULTIPLIER 100
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define NUM_OUT_CHANNELS 12
#define BIND_COUNT 0x1388

volatile s16 Channels[NUM_OUT_CHANNELS];

CYRF6936* CYRF;
char name[24];


//加密
static void scramble_pkt()
{
#ifdef NO_SCRAMBLE
    return;
#else
    u8 i;
    for(i = 0; i < 15; i++) {
        packet[i + 1] ^= cyrfmfg_id[i % 4];
    }
#endif
}

//填写包后缀
static void add_pkt_suffix()
{
    u8 bind_state;
    if (use_fixed_id) {
        if (bind_counter > 0) {
            bind_state = 0xc0;
        } else {
            bind_state = 0x80;
        }
    } else {
        bind_state = 0x00;
    }
    packet[10] = bind_state | (PKTS_PER_CHANNEL - pkt_num - 1);
    packet[11] = *(radio_ch_ptr + 1);
    packet[12] = *(radio_ch_ptr + 2);
    packet[13] = fixed_id  & 0xff;
    packet[14] = (fixed_id >> 8) & 0xff;
    packet[15] = (fixed_id >> 16) & 0xff;
}

//失控保护包
static void build_beacon_pkt(int upper)
{
    packet[0] = ((num_channels << 4) | 0x07);
    u8 enable = 0;
    int max = 8;
    //int offset = 0;
    if (upper) {
        packet[0] += 1;
        max = 4;
        //offset = 8;
    }
    for(int i = 0; i < max; i++) {
        //if (i + offset < Model.num_channels && Model.limits[i+offset].flags & CH_FAILSAFE_EN) {
        //    enable |= 0x80 >> i;
        //    packet[i+1] = Model.limits[i+offset].failsafe;
        //} else {
            packet[i+1] = 0;
        //}
    }
    packet[9] = enable;
    add_pkt_suffix();
}

//绑定包
static void build_bind_pkt()
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

//数据包
static void build_data_pkt()
{
    u8 i;
    packet[0] = (num_channels << 4) | (0x0b + ch_idx);
    u8 sign = 0x0b;
    for (i = 0; i < 4; i++) {
        s32 value = (s32)Channels[ch_idx * 4 + i] * 0x640 / CHAN_MAX_VALUE;
        if(value < 0) {
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

static void cyrf_set_bound_sop_code()
{
    /* crc == 0 isn't allowed, so use 1 if the math results in 0 */
    u8 crc = (cyrfmfg_id[0] + (cyrfmfg_id[1] >> 6) + cyrfmfg_id[2]);
    if(! crc)
        crc = 1;
    u8 sopidx = (0xff &((cyrfmfg_id[0] << 2) + cyrfmfg_id[1] + cyrfmfg_id[2])) % 10;
    CYRF->ConfigRxTx(1);
    CYRF->ConfigCRCSeed((crc << 8) + crc);
    CYRF->ConfigSOPCode(sopcodes[sopidx]);
    CYRF->WriteRegister(CYRF_03_TX_CFG, 0x08 | /*Model.tx_power*/7);
}

//设置频道
void set_radio_channels()
{
	//从4-80信道找出找出3个信号最好的信道，最小间隔4
	CYRF->FindBestChannels(radio_ch, 3, 4, 4, 80);
    radio_ch[3] = radio_ch[0];
    radio_ch[4] = radio_ch[1];
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
			//PROTOCOL_SetBindState(0);
			//oled->print_6x8Str(0,2,"Bind End");
			SEGGER_RTT_printf(0,"Bind End\n");
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
		state=(PktState)(state+1);
		if (bind_counter > 0)
		{
			bind_counter--;
			if (bind_counter == 0)
			{
				//TOTO:PROTOCOL_SetBindState(0);
				//oled->print_6x8Str(0,3,"Bind End in DEVO_BOUND");
				SEGGER_RTT_printf(0,"Bind End in DEVO_BOUND\n");
			}
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

void Init_DEVO()
{
	SEGGER_RTT_printf(0,"---- begin DEVO_Initialize ----\n");
	CLOCK_StopTimer();
	CYRF=new CYRF6936(GPIO_Pin(GPIOA,GPIO_Pin_4),GPIO_Pin(GPIOB,GPIO_Pin_0),SPI1);
	CYRF->CS_HI();
	CYRF->Reset();
	CYRF->WriteRegister(TX_LENGTH_ADR,0x2A);
	u8 r=CYRF->ReadRegister(TX_LENGTH_ADR);
	if(r!=0x2A)
	{
		SEGGER_RTT_printf(0,"cyrf6936 fail!!\n");
		SetLED(CYRF_Fail);
		while(true);
	}
	CYRF->Init();
	//CYRF->ConfigCRCSeed(0xFDFD);
	//CYRF->GetMfgData(cyrfmfg_id);
	//SEGGER_RTT_printf(0,"MFG=%02X %02X %02X %02X %02X %02X\n",
	//		cyrfmfg_id[0],cyrfmfg_id[1],cyrfmfg_id[2],cyrfmfg_id[3],cyrfmfg_id[4],cyrfmfg_id[5]);

	//CYRF->ConfigRxTx(1);
	//CYRF->ConfigCRCSeed(0x0000);
	//CYRF->ConfigSOPCode(sopcodes[0]);
	//set_radio_channels();

	//SEGGER_RTT_printf(0,"CHN=%d %d %d\n",radio_ch[0],radio_ch[1],radio_ch[2]);

	//use_fixed_id = 0;
	//failsafe_pkt = 0;
	//radio_ch_ptr = radio_ch;
	//CYRF->ConfigRFChannel(*radio_ch_ptr);
	//num_channels = ((Model.num_channels + 3) >> 2) * 4;
	//num_channels = (8 + 3)&0xfc; //8ͨ通道
	//pkt_num = 0;
	//ch_idx = 0;
	//txState = 0;

	//伪随机
	//fixed_id = ((u32) (radio_ch[0] ^ cyrfmfg_id[0] ^ cyrfmfg_id[3]) << 16)
	//		| ((u32) (radio_ch[1] ^ cyrfmfg_id[1] ^ cyrfmfg_id[4]) << 8)
	//		| ((u32) (radio_ch[2] ^ cyrfmfg_id[2] ^ cyrfmfg_id[5]) << 0);
	//取模
	//fixed_id = fixed_id % 1000000;
	//bind_counter = BIND_COUNT;
	//state = DEVO_BIND;
	//PROTOCOL_SetBindState(0x1388 * 2400 / 1000); //msecs

	CYRF->ConfigRFChannel(0);
	CYRF->ConfigCRCSeed(0xfdfd);
	CYRF->ConfigSOPCode(sopcodes[0]);
	CYRF->ConfigRxTx(0);
	CYRF->WriteRegister(RX_ABORT_ADR,RX_ABORT_RST);
	CYRF->WriteRegister(RX_CTRL_ADR,RX_CTRL_RST|RX_GO);
	CYRF->WriteRegister(RX_IRQ_STATUS_ADR,0);
	CLOCK_StartTimer(2400, DEVO_Callback);

}

u16 DEVO_Callback()
{
	u8 RX_IRQ_STATUS=CYRF->ReadRegister(RX_IRQ_STATUS_ADR);
	if((RX_IRQ_STATUS&(RXC_IRQ|RXE_IRQ))==RXC_IRQ)
	{
		//SEGGER_RTT_printf(0,"good recv! IRQ=%02X\n",RX_IRQ_STATUS);
		u8 pac[16];
		CYRF->ReadDataPacket(pac);
		RX_IRQ_STATUS=CYRF->ReadRegister(RX_IRQ_STATUS_ADR);
		SEGGER_RTT_printf(0,"%s\t0X%02X\n",pac,RX_IRQ_STATUS);
		u8 RX_STATUS=CYRF->ReadRegister(RX_STATUS_ADR);
		CYRF->WriteRegister(RX_ABORT_ADR,RX_ABORT_RST);
		CYRF->WriteRegister(RX_CTRL_ADR,RX_CTRL_RST|RX_GO);
		//CYRF->WriteRegister(RX_IRQ_STATUS_ADR,0);
	}
	return 240;
	if (txState == 0)
	{
		//发送数据包
		txState = 1;
		DEVO_BuildPacket();
		CYRF->WriteDataPacket(packet);
		return 1200;
	}
	txState = 0;
	int i = 0;
	//检查发送状态
	u8 IRQ_STATUS = 0;
	while (true)
	{
		//first read
		IRQ_STATUS = CYRF->ReadRegister(TX_IRQ_STATUS_ADR);
		if((IRQ_STATUS&(TXC_IRQ|TXBERR_IRQ|TXE_IRQ))==TXC_IRQ)
		{
			//second read
			IRQ_STATUS = CYRF->ReadRegister(TX_IRQ_STATUS_ADR);
			if((IRQ_STATUS&TXE_IRQ)==0)
			{
				//Successful Transactions
				break;
			}
		}
		if(++i > NUM_WAIT_LOOPS)
			return 1200;//如果发送失败1200us后重新发送
	}
	i=CYRF->ReadRegister(TX_LENGTH_ADR);
	//SEGGER_RTT_printf(0,"Successful Transactions\n");
	if (state == DEVO_BOUND)//已经绑定成功
	{
		/* exit binding state */
		state = DEVO_BOUND_3;
		cyrf_set_bound_sop_code();
	}
	if (pkt_num == 0)
	{
		//Keep tx power updated
		CYRF->WriteRegister(CYRF_03_TX_CFG, 0x08 | /*Model.tx_power*/7);
		radio_ch_ptr = ((radio_ch_ptr == &radio_ch[2]) ? radio_ch : radio_ch_ptr + 1);
		CYRF->ConfigRFChannel(*radio_ch_ptr);
	}
	return 1200;
}

