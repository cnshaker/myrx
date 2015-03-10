#include "OLED.h"
#include "delay.h"
#include "codetab.h"
#include "stm32f10x_gpio.h"

void OLED::SetPos(unsigned char x, unsigned char y)
{
	ChipSelect_Begin;
	WC(0xb0 + y);
	WC(((x & 0xf0) >> 4) | 0x10);
	WC((x & 0x0f) | 0x01);
	ChipSelect_End;
}

void OLED::Fill(unsigned char bmp_dat)
{
	ChipSelect_Begin;
	unsigned char y, x;
	for (y = 0; y < 8; y++)
	{
		WC(0xb0 + y);
		WC(0x01);
		WC(0x10);
		for (x = 0; x < X_WIDTH; x++)
		{
			WD(bmp_dat);
		}
	}
	ChipSelect_End;
}

void OLED::Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = cs_pin | dc_pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(gpiox, &GPIO_InitStructure);
	GPIO_SetBits(gpiox, dc_pin);
	GPIO_SetBits(gpiox, cs_pin);

	delay_ms(1500);

	{
		//GPIO_ResetBits(gpiox, cs_pin);
		ChipSelect_Begin;
		WC(0xae);
		WC(0xae); //--turn off oled panel
		WC(0x00); //---set low column address
		WC(0x10); //---set high column address
		WC(0x40); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
		WC(0x81); //--set contrast control register
		WC(0x10); // Set SEG Output Current Brightness
		WC(0xa1); //--Set SEG/Column Mapping     0xa0,0xa1
		WC(0xc8); //Set COM/Row Scan Direction   0xc0,0xc8
		WC(0xa6); //--set normal display
		WC(0xa8); //--set multiplex ratio(1 to 64)
		WC(0x3f); //--1/64 duty
		WC(0xd3); //-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
		WC(0x00); //-not offset
		WC(0xd5); //--set display clock divide ratio/oscillator frequency
		WC(0x80); //--set divide ratio, Set Clock as 100 Frames/Sec
		WC(0xd9); //--set pre-charge period
		WC(0xf1); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
		WC(0xda); //--set com pins hardware configuration
		WC(0x12);
		WC(0xdb); //--set vcomh
		WC(0x40); //Set VCOM Deselect Level
		WC(0x20); //-Set Page Addressing Mode (0x00/0x01/0x02)
		WC(0x02); //
		WC(0x8d); //--set Charge Pump enable/disable
		WC(0x14); //--set(0x10) disable
		WC(0xa4); // Disable Entire Display On (0xa4/0xa5)
		WC(0xa6); // Disable Inverse Display On (0xa6/a7)
		WC(0xaf); //--turn on oled panel
		Fill(0x00);
		SetPos(0, 0);
		ChipSelect_End;
	}
}

void OLED::print_6x8Str(unsigned char x, unsigned char y, const char *ch)
{
	ChipSelect_Begin;
	unsigned char c = 0, i = 0, j = 0;
	while (ch[j] != '\0')
	{
		c = ch[j] - 32;
		if (x > 126)
		{
			x = 0;
			y++;
		}
		SetPos(x, y);
		for (i = 0; i < 6; i++)
		{
			WD(F6x8[c][i]);
		}
		x += 6;
		j++;
	}
	ChipSelect_End;
}

void OLED::print_8x16Str(unsigned char x, unsigned char y, const char *ch)
{
	ChipSelect_Begin;
	unsigned char c = 0, i = 0, j = 0;
	while (ch[j] != '\0')
	{
		c = ch[j] - 32;
		if (x > 120)
		{
			x = 0;
			y++;
		}
		SetPos(x, y);
		for (i = 0; i < 8; i++)
		{
			WD(F8X16[c * 16 + i]);
		}
		SetPos(x, y + 1);
		for (i = 0; i < 8; i++)
		{
			WD(F8X16[c * 16 + i + 8]);
		}
		x += 8;
		j++;
	}
	ChipSelect_End;
}

void OLED::print_16x16CN(unsigned char x, unsigned char y, unsigned char N)
{
	ChipSelect_Begin;
	unsigned char wm = 0;
	unsigned int adder = 32 * N;
	SetPos(x, y);
	for (wm = 0; wm < 16; wm++)
	{
		WD(F16x16[adder]);
		adder += 1;
	}
	SetPos(x, y + 1);
	for (wm = 0; wm < 16; wm++)
	{
		WD(F16x16[adder]);
		adder += 1;
	}
	ChipSelect_End;
}

void OLED::print_BMP(unsigned char x0, unsigned char y0, unsigned char x1,
		unsigned char y1, const char *BMP)
{
	ChipSelect_Begin;
	unsigned int j = 0;
	unsigned char x, y;

	if (y1 % 8 == 0)
	{
		y = y1 / 8;
	}
	else
		y = y1 / 8 + 1;
	for (y = y0; y < y1; y++)
	{
		SetPos(x0, y);
		for (x = x0; x < x1; x++)
		{
			WD(BMP[j++]);
		}
	}
	ChipSelect_End;
}
