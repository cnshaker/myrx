#include "main.h"
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include "delay.h"
#include "OLED.h"
#include "Timx.h"

void Init_SPI(void)
{
	/* Enable GPIOA and SPI1 clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
	/* SCK, MOSI */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* MISO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* SS  PA2 for RF   PA3 for OLED */
	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
}

int main(int argc, char* argv[])
{
	Init_SPI();
	delay_init();

	//GPIO_Pin_13 for led
	//GPIO_Pin_1 for oled's chip select
	//GPIO_Pin_2 for oled's data/command
	//GPIO_Pin_3 for cyrf6936's chip select
	//GPIO_Pin_4 for cyrf's reset
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_1 | GPIO_Pin_2	| GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_3);

	OLED oled(GPIOA, GPIO_Pin_1, GPIO_Pin_2);
	oled.Init();

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	TIM3_Int_Init(10000-1,7200-1);//10Khz的计数频率，计数到5000为500ms

	while (1)
	{
		//GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		//oled.CLS();
		for (int i = 0; i < 8; i++)
		{
			oled.print_16x16CN(i * 16, 0, i);
			oled.print_16x16CN(i * 16, 2, i + 8);
			oled.print_16x16CN(i * 16, 4, i + 16);
			oled.print_16x16CN(i * 16, 6, i + 24);
		}
		//GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_ms(2000);		// Add your code here.
	}
}

// ----------------------------------------------------------------------------
