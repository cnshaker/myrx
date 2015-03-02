/************************************************************************************
 *  Copyright (c), 2014, HelTec Automatic Technology co.,LTD.
 *            All rights reserved.
 *
 * Http:    www.heltec.cn
 * Email:   cn.heltec@gmail.com
 * WebShop: heltec.taobao.com
 *
 * File name: OLED.h
 * Project  : HelTec.uvprij
 * Processor: STM32F103C8T6
 * Compiler : MDK fo ARM
 * 
 * Author : 小林
 * Version: 1.00
 * Date   : 2014.2.20
 * Email  : hello14blog@gmail.com
 * Modification: none
 * 
 * Description:128*64点阵的OLED显示屏驱动文件，仅适用于惠特自动化(heltec.taobao.com)的SD1306驱动SPI通信方式显示屏
 *
 * Others: none;
 *
 * Function List:
 *
 * 2. void OLED_WrDat(unsigned char dat) -- 向OLED写数据
 * 3. void OLED_WrCmd(unsigned char cmd) -- 向OLED写命令
 * 4. void OLED_SetPos(unsigned char x, unsigned char y) -- 设置起始点坐标
 * 5. void OLED_Fill(unsigned char bmp_dat) -- 全屏填充(0x00可以用于清屏，0xff可以用于全屏点亮)
 * 6. void OLED_CLS(void) -- 清屏
 * 7. void OLED_Init(void) -- OLED显示屏初始化
 * 8. void OLED_6x8Str(unsigned char x, y,unsigned char ch[]) -- 显示6x8的ASCII字符
 * 9. void OLED_8x16Str(unsigned char x, y,unsigned char ch[]) -- 显示8x16的ASCII字符
 * 10.void OLED_16x16CN(unsigned char x, y, N) -- 显示16x16中文汉字,汉字要先在取模软件中取模
 * 11.void OLED_BMP(unsigned char x0, y0,x1, y1,unsigned char BMP[]) -- 全屏显示128*64的BMP图片
 *
 * History: none;
 *
 *************************************************************************************/

#ifndef __OLED_H
#define __OLED_H
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"

#define XLevelL			0x00
#define XLevelH			0x10
#define XLevel	    ((XLevelH&0x0F)*16+XLevelL)
#define Max_Column	128
#define Max_Row			64
#define	Brightness	0xCF 
#define X_WIDTH 		128
#define Y_WIDTH 		64

class OLED
{
	class AutoChipSelect
	{
	public:
		AutoChipSelect(OLED *parent) :
				oled(parent), old_status(
						GPIO_ReadOutputDataBit(parent->gpiox, parent->cs_pin))
		{
			if (old_status != Bit_RESET)
			{
				GPIO_ResetBits(oled->gpiox, oled->cs_pin);
				GPIO_SetBits(GPIOC, GPIO_Pin_13);
			}
		}
		~AutoChipSelect()
		{
			if (old_status != Bit_RESET)
			{
				GPIO_SetBits(oled->gpiox, oled->cs_pin);
				GPIO_ResetBits(GPIOC, GPIO_Pin_13);
			}
		}
	private:
		OLED *oled;
		uint8_t old_status;
	};
public:
	OLED() :
			gpiox(GPIOA), cs_pin(1), dc_pin(2)
	{
	}
	OLED(GPIO_TypeDef* GPIOx, uint16_t cs, uint16_t dc) :
			gpiox(GPIOx), cs_pin(cs), dc_pin(dc)
	{
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

private:
	GPIO_TypeDef *gpiox;
	uint16_t cs_pin;
	uint16_t dc_pin;
	inline void DC_Clr()
	{
		GPIO_ResetBits(gpiox, dc_pin);
	}
	inline void DC_Set()
	{
		GPIO_SetBits(gpiox, dc_pin);
	}
	inline void WB(unsigned char byt)
	{
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
			;
		SPI_I2S_SendData(SPI1, byt);
	}
	void WD(unsigned char dat)
	{
		DC_Set();
		WB(dat);
	}
	void WC(unsigned char cmd)
	{
		DC_Clr();
		WB (cmd);
	}
};

#endif
