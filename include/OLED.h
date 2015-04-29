#ifndef __OLED_H
#define __OLED_H
#include "main.h"

#define XLevelL			0x00
#define XLevelH			0x10
#define XLevel	    ((XLevelH&0x0F)*16+XLevelL)
#define Max_Column	128
#define Max_Row			64
#define	Brightness	0xCF 
#define X_WIDTH 		128
#define Y_WIDTH 		64

#define ChipSelect_Begin	uint8_t old_status=cs.gpiox->ODR&cs.pin;\
														if(old_status!=Bit_RESET)\
														cs.gpiox->BRR=cs.pin;
#define ChipSelect_End		if(old_status!=Bit_RESET)\
														cs.gpiox->BSRR=cs.pin;

class OLED
{
public:
	OLED(GPIO_Pin cs_pin, GPIO_Pin dc_pin) :
			cs(cs_pin), dc(dc_pin)
	{
		StartLine(0);
	}
	void SetPos(unsigned char x, unsigned char y);
	void Fill(unsigned char bmp_dat); //全屏填充
	inline void CLS(void)
	{
		Fill(0x00);
	}
	void Init();
	void print_6x8Str(unsigned char x, unsigned char y, const char* ch);
	void print_8x16Str(unsigned char x, unsigned char y, const char* ch);
	void print_16x16CN(unsigned char x, unsigned char y, unsigned char N);
	void print_BMP(unsigned char x0, unsigned char y0, unsigned char x1,
			unsigned char y1, const char *BMP);
	void StartLine(int i)
	{
		ChipSelect_Begin;
		start_line=i;
		WC(0x40+i);
		ChipSelect_End;
	}

private:
	GPIO_Pin  cs;
	GPIO_Pin  dc;
	u8 start_line;
	inline void DC_Clr()
	{
		dc.gpiox->BRR = dc.pin;
	}
	inline void DC_Set()
	{
		dc.gpiox->BSRR = dc.pin;
	}
	inline u8 WB(u8 byt)
	{
		while ((SPI1->SR & SPI_I2S_FLAG_TXE) == RESET);
		SPI1->DR = byt;
		while ((SPI1->SR & SPI_I2S_FLAG_RXNE) == RESET);
		return SPI1->DR;
	}
	inline void WD(unsigned char dat)
	{
		DC_Set();
		WB(dat);
	}
	inline void WC(unsigned char cmd)
	{
		DC_Clr();
		WB(cmd);
	}
};

extern OLED *oled;

#endif
