#ifndef __CYRF_H__
#define __CYRF_H__
#include <stm32f10x.h>
enum CYRF_REGISTER_ADDR
{
	CYRF_00_CHANNEL = 0x01,
	CYRF_01_TX_LENGTH = 0x01,
	CYRF_02_TX_CTRL = 0x02,
	CYRF_03_TX_CFG = 0x03,
	CYRF_04_TX_IRQ_STATUS = 0x04,
	CYRF_05_RX_CTRL = 0x05,
	CYRF_06_RX_CFG = 0x06,
	CYRF_07_RX_IRG_STATUS = 0x07,
	CYRF_08_RX_STATUS = 0x08,
	CYRF_09_RX_COUNT = 0x09,
	CYRF_0A_RX_LENGTH = 0x0A,
	CYRF_0B_PWR_CTRL = 0x0B,
	CYRF_0C_XTAL_CTRL = 0x0C,
	CYRF_0D_IO_CFG = 0x0D,
	CYRF_0E_GPIO_CTRL = 0x0E,
	CYRF_0F_XACT_CFG = 0x0F,
	CYRF_10_FRAMING_CFG = 0x10,
	CYRF_11_DATA32_THOLD = 0x11,
	CYRF_12_DATA64_THOLD = 0x12,
	CYRF_13_RSSI = 0x13,
	CYRF_14_EOP_CTRL = 0x14,
	CYRF_15_CRC_SEED_LSB = 0x15,
	CYRF_16_CRC_SEED_MSB = 0x16,
	CYRF_17_TX_CRC_LSB = 0x17,
	CYRF_18_TX_CRC_MSB = 0x18,
	CYRF_19_RX_CRC_LSB = 0x19,
	CYRF_1A_RX_CRC_MSB = 0x1A,
	CYRF_1B_TX_OFFSET_LSB = 0x1B,
	CYRF_1C_TX_OFFSET_MSB = 0x1C,
	CYRF_1D_MODE_OVERRIDE = 0x1D,
	CYRF_1E_RX_OVERRIDE = 0x1E,
	CYRF_1F_TX_OVERRIDE = 0x1F,
	CYRF_20_TX_BUFFER = 0x20,
	CYRF_21_RX_BUFFER = 0x21,
	CYRF_22_SOP_CODE = 0x22,
	CYRF_23_DATA_CODE = 0x23,
	CYRF_24_PREAMBLE = 0x24,
	CYRF_25_MFG_ID = 0x25,
	CYRF_26_XTAL_CFG = 0x26,
	CYRF_27_CLK_OVERRIDE = 0x27,
	CYRF_28_CLK_EN = 0x28,
	CYRF_29_RX_ABORT = 0x29,
	CYRF_32_AUTO_CAL_TIME = 0x32,
	CYRF_35_AUTOCAL_OFFSET = 0x35,
	CYRF_39_ANALOG_CTRL = 0x39,
};

const u8 CYRF_WRITE=0x80;
const u8 CYRF_ADR_MASK=0x3F;
const u8 RX_GO=0x80;

class CYRF6936
{
private:
	GPIO_TypeDef *gpiox;
	uint16_t cs_pin;
	uint16_t reset_pin;
	SPI_TypeDef* spix;
public:
	static const u8 sopcodes[][8];
	CYRF6936(GPIO_TypeDef*gpio,uint16_t cs,uint16_t reset,SPI_TypeDef* spi)
		: gpiox(gpio),cs_pin(cs),reset_pin(reset),spix(spi)
	{
		CS_HI();
	}
	virtual ~CYRF6936(){}
	inline void CS_LO()
	{
		GPIO_ResetBits(gpiox, cs_pin);
	}

	inline void CS_HI()
	{
		GPIO_SetBits(gpiox, cs_pin);
	}

	inline void RS_LO()
	{
		GPIO_ResetBits(gpiox, reset_pin);
	}

	inline void RS_HI()
	{
		GPIO_SetBits(gpiox, reset_pin);
	}

	inline u8 transfer(u8 byt)
	{
		while (!(spix->SR & SPI_SR_TXE));
		spix->DR = byt;
		while (!(spix->SR & SPI_SR_RXNE));
		return spix->DR;
	}
	void WriteRegister(u8 address, u8 data);
	void WriteRegisterMulti(u8 address, const u8 data[], u8 length);
	void ReadRegisterMulti(u8 address, u8 data[], u8 length);
	u8 ReadRegister(u8 address);
	void Reset();
	void ConfigRxTx(u32 TxRx);//1 - Tx else Rx
	void ConfigCRCSeed(u16 crc);
	void ConfigSOPCode(const u8 *sopcodes);
	void ConfigDataCode(const u8 *datacodes, u8 len);
	void WritePreamble(unsigned long preamble);
	void Init();
	void GetMfgData(u8 data[]);
	void StartReceive();
	u8 ReadRSSI(unsigned long dodummyread);
	void ConfigRFChannel(u8 ch);
	void FindBestChannels(u8 *channels, u8 len, u8 minspace, u8 min, u8 max);
	void ReadDataPacket(u8 dpbuffer[]);
	void WriteDataPacketLen(const u8 dpbuffer[], u8 len);
	void WriteDataPacket(const u8 dpbuffer[]);
	void SetPower(u8 power);
};
#endif

