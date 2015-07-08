#include "main.h"
#include "delay.h"
#include "CYRF.h"
#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include "segger_rtt.h"
/*
 nSS * * SCK
 IRQ * * MOSI
 MISO * * RST
 XOUT * * PACTL
 FGND * * VCC
 RX_EN * * TX_EN  
 */

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
	address |= CYRF_READ;
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
	address |= CYRF_READ;
	u8 data;
	CS_LO();
	transfer(address);
	data = transfer(address);
	CS_HI();
	return data;
}

void CYRF6936::Reset()
{
	/* Reset the CYRF chip */
	RS_HI();
	delay_us(200);
	RS_LO();
	delay_us(200);
}

void CYRF6936::ConfigRxTx(u32 TxRx)
{
	if(TxRx!=0)
	{
		//oled->print_6x8Str(0, 3, "Tx Mode");
		SEGGER_RTT_printf(0,"Tx Mode\n");
		WriteRegister(CYRF_0E_GPIO_CTRL, PACTL_OP);
		WriteRegister(CYRF_0F_XACT_CFG,
				FRC_END_STATE | END_STATE_RXSYNTH);
	}
	else
	{
		//oled->print_6x8Str(0, 3, "Rx Mode");
		SEGGER_RTT_printf(0,"Rx Mode\n");
		WriteRegister(CYRF_0E_GPIO_CTRL, XOUT_OP);
		WriteRegister(CYRF_0F_XACT_CFG,
				FRC_END_STATE | END_STATE_TXSYNTH);
	}
}

void CYRF6936::ConfigCRCSeed(u16 crc)
{
	WriteRegister(CRC_SEED_LSB_ADR, crc & 0xff);
	WriteRegister(CRC_SEED_MSB_ADR, crc >> 8);
}

void CYRF6936::ConfigSOPCode(const u8 *sopcodes)
{

	WriteRegisterMulti(SOP_CODE_ADR, sopcodes, 8);
}

void CYRF6936::ConfigDataCode(const u8 *datacodes, u8 len)
{

	WriteRegisterMulti(DATA_CODE_ADR, datacodes, len);
}

void CYRF6936::WritePreamble(unsigned long preamble)
{
	CS_LO();
	transfer((CYRF_ADR_MASK & PREAMBLE_ADR) | CYRF_WRITE);
	transfer(preamble & 0xff);
	transfer((preamble >> 8) & 0xff);
	transfer((preamble >> 16) & 0xff);
	CS_HI();
}

void CYRF6936::Init()
{
	/* Initialise CYRF chip */
	WriteRegister(MODE_OVERRIDE_ADR, FRC_SEN|MODE_OVRD_FRC_AWAKE|RST);
	WriteRegister(TX_CFG_ADR, DATMODE_8DR|PA_4_DBM);
	WriteRegister(RX_CFG_ADR, LNA_EN|FASTTURN_EN|RXOW_EN);
	WriteRegister(PWR_CTRL_ADR, 0x00);
	WriteRegister(IO_CFG_ADR, PACTL_GPIO);
	WriteRegister(GPIO_CTRL_ADR, PACTL_OP);
	WriteRegister(FRAMING_CFG_ADR, SOP_EN|LEN_EN|0x04);
	WriteRegister(DATA32_THOLD_ADR, 0x05);
	WriteRegister(DATA64_THOLD_ADR, 0x0E);
	WriteRegister(TX_OFFSET_LSB_ADR, 0x55);
	WriteRegister(TX_OFFSET_MSB_ADR, 0x05);
	WriteRegister(AUTO_CAL_TIME_ADR, AUTO_CAL_TIME_MAX);
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

	SEGGER_RTT_printf(0,"Channel=%d\n",ch);

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

	memset(channels, 0, sizeof(u8) * len);
	ConfigCRCSeed(0x0000);
	ConfigRxTx(0);
	//Wait for pre-amp to switch from sned to receive
	delay_us(1000);
	for(int i=0;i<NUM_FREQ;i++)
	{
		ConfigRFChannel(i);
		ReadRegister(CYRF_13_RSSI);
		StartReceive();
		delay_us(10);
		rssi[i] = ReadRegister(CYRF_13_RSSI);
	}

	for(int i=0;i<len;i++)
	{
		channels[i] = min;
		for (int j=min;j<max;j++)
		{
			if (rssi[j] < rssi[channels[i]])
			{
				channels[i] = j;
			}

		}
		for (int j=channels[i]-minspace;j<channels[i]+minspace;j++)
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
	ReadRegisterMulti(RX_BUFFER_ADR, dpbuffer, 0x10);
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

