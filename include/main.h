#ifndef _MAIN_H_
#define _MAIN_H_
#include <stm32f10x.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_gpio.h>
#include <stdio.h>

class GPIO_Pin
{
public:
	GPIO_Pin(GPIO_TypeDef*x,uint16_t p):gpiox(x),pin(p){}
	GPIO_TypeDef* gpiox;
	uint16_t pin;
	GPIO_Pin& operator = (const GPIO_Pin& rhs)
	{
		gpiox = rhs.gpiox;
		pin = rhs.pin;
		return *this;
	}
};

extern int led_fast_flash, led_slow_flash;

enum LED_Flash_Type
{
	Initing,
	Working,
	CYRF_Fail,
};
void SetLED(LED_Flash_Type);
#endif
