/*
 * Output.cpp
 *
 *  Created on: 2015年7月23日
 *      Author: shaker
 */

#include <stm32f10x.h>
#include "Output.h"

Output::Output()
{
}

Output::~Output()
{
}

void Output::Init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);	//使能定时器2时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE); //使能GPIO外设和AFIO复用功能模块时钟

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	//TIM2_CH1 --> PA0
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; //TIM_CH1
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化GPIO

	//TIM2_CH2 --> PA1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //TIM_CH2
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化GPIO

	//TIM2_CH3 --> PA2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //TIM_CH3
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化GPIO

	//TIM2_CH4 --> PA3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //TIM_CH4
	GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化GPIO

	//定时器TIM2初始化
	TIM_TimeBaseStructure.TIM_Period = 20000 - 1; //20ms
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	//初始化TIM2_CH1 PWM模式
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择定时器模式:TIM脉冲宽度调制模式2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM2 OC1
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM2 OC2
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM2 OC3
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM2 OC4

	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_Cmd(TIM2, ENABLE);  //使能TIM3
	
	
	SetChannelValue(0,0);
	SetChannelValue(1,2000);
	SetChannelValue(2,1000);
	SetChannelValue(3,4000);
}

void Output::SetChannelValue(u8 idx, u16 val)
{
	if(val>2000)val=2000;
	if (idx < MAX_CHANNEL)
	{
		Channels[idx] = val;
		switch (idx)
		{
		case 0:
			TIM_SetCompare1(TIM2, val + 1000);
			break;
		case 1:
			TIM_SetCompare2(TIM2, val + 1000);
			break;
		case 2:
			TIM_SetCompare3(TIM2, val + 1000);
			break;
		case 3:
			TIM_SetCompare4(TIM2, val + 1000);
			break;
		default:
			break;
		}
	}
}
