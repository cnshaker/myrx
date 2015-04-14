#include "main.h"
#include "delay.h"
#include "CYRF.h"
#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
/*
 nSS * * SCK
 IRQ * * MOSI
 MISO * * RST
 XOUT * * PACTL
 FGND * * VCC
 RX_EN * * TX_EN  
 */
const u8 CYRF6936::sopcodes[][8] =
{
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

void CYRF6936::WriteRegister(u8 address, u8 data)
{
	address &= CYRF_ADR_MASK;
	address |= CYRF_WRITE;
	CS_LO();
	transfer(address);
	transfer(data);
	CS_HI();
}

void CYRF6936::WriteRegisterMulti(u8 address, const u8 data[], u8 length)
{
	address &= CYRF_ADR_MASK;
	address |= CYRF_WRITE;
	unsigned char i;
	CS_LO();
	transfer(address);
	for (i = 0; i < length; i++)
	{
		transfer(data[i]);
	}
	CS_HI();
}

void CYRF6936::ReadRegisterMulti(u8 address, u8 data[], u8 length)
{
	address &= CYRF_ADR_MASK;
	unsigned char i;
	CS_LO();
	transfer(address);
	for (i = 0; i < length; i++)
	{
		data[i] = transfer(0);
	}
	CS_HI();
}

u8 CYRF6936::ReadRegister(u8 address)
{
	address &= CYRF_ADR_MASK;
	u8 data;

	CS_LO();
	transfer(address);
	data = transfer(0);
	CS_HI();
	return data;
}

void CYRF6936::Reset()
{
	/* Reset the CYRF chip */
	RS_HI();
	delay_us(100);
	RS_LO();
	delay_us(100);
}

void CYRF6936::ConfigRxTx(u32 TxRx)
{
	if (TxRx)
	{
		WriteRegister(CYRF_0E_GPIO_CTRL, 0x80);
		WriteRegister(CYRF_0F_XACT_CFG, 0x2C);
	}
	else
	{
		WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
		WriteRegister(CYRF_0F_XACT_CFG, 0x28);
	}
}

void CYRF6936::ConfigCRCSeed(u16 crc)
{
	WriteRegister(0x15, crc & 0xff);
	WriteRegister(0x16, crc >> 8);
}

void CYRF6936::ConfigSOPCode(const u8 *sopcodes)
{

	WriteRegisterMulti(CYRF_22_SOP_CODE, sopcodes, 8);
}

void CYRF6936::ConfigDataCode(const u8 *datacodes, u8 len)
{

	WriteRegisterMulti(CYRF_23_DATA_CODE, datacodes, len);
}

void CYRF6936::WritePreamble(unsigned long preamble)
{
	CS_LO();
	transfer((CYRF_ADR_MASK & CYRF_24_PREAMBLE) | CYRF_WRITE);
	transfer(preamble & 0xff);
	transfer((preamble >> 8) & 0xff);
	transfer((preamble >> 16) & 0xff);
	CS_HI();
}

void CYRF6936::Init()
{
	/* Initialise CYRF chip */
	WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x39);
	WriteRegister(CYRF_03_TX_CFG, 0x08 | 7);
	WriteRegister(CYRF_06_RX_CFG, 0x4A);
	WriteRegister(CYRF_0B_PWR_CTRL, 0x00);
	WriteRegister(CYRF_0D_IO_CFG, 0x04);
	WriteRegister(CYRF_0E_GPIO_CTRL, 0x20);
	WriteRegister(CYRF_10_FRAMING_CFG, 0xA4);
	WriteRegister(CYRF_11_DATA32_THOLD, 0x05);
	WriteRegister(CYRF_12_DATA64_THOLD, 0x0E);
	WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);
	WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);
	WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);
	WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);
	WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);
	WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);
	WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);
	WriteRegister(CYRF_01_TX_LENGTH, 0x10);
	WriteRegister(CYRF_0C_XTAL_CTRL, 0xC0);
	WriteRegister(CYRF_0F_XACT_CFG, 0x10);
	WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);
	WriteRegister(CYRF_28_CLK_EN, 0x02);
	WriteRegister(CYRF_0F_XACT_CFG, 0x28);
}

void CYRF6936::GetMfgData(u8 data[])
{
	/* Fuses power on */
	WriteRegister(CYRF_25_MFG_ID, 0xFF);

	ReadRegisterMulti(CYRF_25_MFG_ID, data, 6);

	/* Fuses power off */
	WriteRegister(CYRF_25_MFG_ID, 0x00);
}

void CYRF6936::StartReceive()
{
	WriteRegister(CYRF_05_RX_CTRL, 0x87);
}

u8 CYRF6936::ReadRSSI(unsigned long dodummyread)
{
	u8 result;
	if (dodummyread)
	{
		result = ReadRegister(CYRF_13_RSSI);
	}
	result = ReadRegister(CYRF_13_RSSI);
	if (result & 0x80)
	{
		result = ReadRegister(CYRF_13_RSSI);
	}
	return (result & 0x0F);
}
u8 channel;
void CYRF6936::ConfigRFChannel(u8 ch)
{
	channel=ch;
	WriteRegister(CYRF_00_CHANNEL, ch);
}

//NOTE: This routine will reset the CRC Seed
void CYRF6936::FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 min,
		u8 max)
{
	const int NUM_FREQ = 80;
	const int FREQ_OFFSET = 4;
	u8 rssi[NUM_FREQ];

	if (min < FREQ_OFFSET)
		min = FREQ_OFFSET;
	if (max > NUM_FREQ)
		max = NUM_FREQ;

	int i;
	int j;
	memset(channels, 0, sizeof(u8) * len);
	ConfigCRCSeed(0x0000);
	ConfigRxTx(0);
	//Wait for pre-amp to switch from sned to receive
	delay_us(1000);
	for (i = 0; i < NUM_FREQ; i++)
	{
		ConfigRFChannel(i);
		ReadRegister(CYRF_13_RSSI);
		StartReceive();
		delay_us(10);
		rssi[i] = ReadRegister(CYRF_13_RSSI);
	}

	for (i = 0; i < len; i++)
	{
		channels[i] = min;
		for (j = min; j < max; j++)
		{
			if (rssi[j] < rssi[channels[i]])
			{
				channels[i] = j;
			}

		}
		for (j = channels[i] - minspace; j < channels[i] + minspace; j++)
		{
			//Ensure we don't reuse any channels within minspace of the selected channel again
			if (j < 0 || j >= NUM_FREQ)
				continue;
			rssi[j] = 0xff;
		}
	}
	ConfigRxTx(1);
}

void CYRF6936::ReadDataPacket(u8 dpbuffer[])
{
	ReadRegisterMulti(CYRF_21_RX_BUFFER, dpbuffer, 0x10);
}

void CYRF6936::WriteDataPacketLen(const u8 dpbuffer[], u8 len)
{
	WriteRegister(CYRF_01_TX_LENGTH, len);
	WriteRegister(CYRF_02_TX_CTRL, 0x40);
	WriteRegisterMulti(CYRF_20_TX_BUFFER, dpbuffer, len);
	WriteRegister(CYRF_02_TX_CTRL, 0xBF);
}

void CYRF6936::WriteDataPacket(const u8 dpbuffer[])
{
	WriteDataPacketLen(dpbuffer, 16);
}

void CYRF6936::SetPower(u8 power)
{
	u8 val = ReadRegister(CYRF_03_TX_CFG) & 0xF8;
	WriteRegister(CYRF_03_TX_CFG, val | (power & 0x07));
}

