#include "main.h"
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include "delay.h"
#include "Timx.h"
#include "cyrf.h"
#include "devo.h"
#include "SEGGER_RTT.h"

void Init_SPI(void)
{
	/* Enable GPIOA and SPI1 clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_SPI1, ENABLE);

	//cyrf6936's GPIO
	//PA4 for NSS
	//PA5 for SCK
	//PA6 for MISO
	//PA7 for MOSI
	//PB0 for RST

	/* SPI1: SCK, MOSI */
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	/* SPI1: MISO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//PA4
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
	//PB0
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);

	SPI_InitTypeDef SPI_InitStructure;
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
}

int led_fast_flash, led_slow_flash;
void Init_LED(void)
{
	// Enable GPIOC for LED
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	// PC13 for led
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	led_fast_flash=3;
	led_slow_flash=1;
	Init_TIM3(1000-1,7200-1);//10Khz的计数频率

}

void SetLED(LED_Flash_Type t)
{
	switch(t)
	{
		case Initing: led_fast_flash=1000;led_slow_flash=0;break;
		case Working: led_fast_flash=1;led_slow_flash=0;break;
		case CYRF_Fail: led_fast_flash=2;led_slow_flash=1;break;
		case RF_Bound: led_fast_flash=2;led_slow_flash=0;break;
		case RF_Lost: led_fast_flash=3;led_slow_flash=1;break;
	}
}

int main(int argc, char* argv[])
{
	SEGGER_RTT_Init();
	SEGGER_RTT_printf(0,"\n\nstarting...\n");

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	Init_LED();
	SetLED(Initing);
	Init_SPI();
	Init_Delay();
	Init_Clock();
	DEVO devo;
	pDEVO=&devo;
	devo.Init();
	SetLED(Working);

	while (1)
	{
	}
}

// ----------------------------------------------------------------------------
