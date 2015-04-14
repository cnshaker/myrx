/*
 * TimX.cpp
 *
 *  Created on: 2015年3月10日
 *      Author: shaker
 */
#include <stm32f10x.h>
#include <stm32f10x_tim.h>
#include <stdio.h>
#include "TimX.h"
#include "oled.h"

//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM3, ENABLE);  //使能TIMx
}

//定时器3中断服务程序
int ttt=0;
extern "C" void TIM3_IRQHandler(void) //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx更新中断标志
		if(GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13))
			GPIO_ResetBits(GPIOC, GPIO_Pin_13);
		else
		{
			GPIO_SetBits(GPIOC, GPIO_Pin_13);
			char buf[8];
			ttt++;
			sprintf(buf,"%d",ttt);
			oled->print_6x8Str(0,6,buf);

		}
	}
}

timer_callback_t timer_callback=0;

extern "C" void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET)  //检查TIM3更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
		if (timer_callback)
		{
			u16 us = timer_callback();
			TIM_ClearFlag(TIM4, TIM_FLAG_CC1);
			if (us)
			{
				TIM_SetCompare1(TIM4, us + TIM_GetCapture1(TIM4));
				return;
			}
		}
		//CLOCK_StopTimer();
	}
}

void CLOCK_Init()
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	/* Setup timer for Transmitter */
	timer_callback = 0;
	/* Enable TIM4 clock. */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能

	/* Enable TIM4 interrupt. */
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM4, DISABLE);

	TIM_DeInit(TIM4);

	//TIM4
	TIM_TimeBaseStructure.TIM_Period = 65535; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

	/* Disable preload. */
	TIM_ARRPreloadConfig(TIM4, DISABLE);

	/* Continous mode. */
	TIM_SelectOnePulseMode(TIM4, TIM_OPMode_Repetitive);

	/* Disable outputs. */
  //timer_disable_oc_output(TIM4, TIM_OC1);
  //timer_disable_oc_output(TIM4, TIM_OC2);
  //timer_disable_oc_output(TIM4, TIM_OC3);
  //timer_disable_oc_output(TIM4, TIM_OC4);
  TIM4->CCER &= ~(TIM_CCER_CC1E|TIM_CCER_CC2E|TIM_CCER_CC3E|TIM_CCER_CC4E);

	/* Enable CCP1 */
	//timer_disable_oc_clear(TIM4, TIM_OC1);
	//timer_disable_oc_preload(TIM4, TIM_OC1);
	//timer_set_oc_slow_mode(TIM4, TIM_OC1);
	//timer_set_oc_mode(TIM4, TIM_OC1, TIM_OCM_FROZEN);
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCStructInit(&TIM_OCInitStructure);
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM4, & TIM_OCInitStructure);
	TIM_ClearOC1Ref(TIM4, TIM_OCClear_Disable);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Disable);
	TIM_OC1FastConfig(TIM2, TIM_OCFast_Disable);

	/* Disable CCP1 interrupt. */
	//timer_disable_irq(TIM4, TIM_DIER_CC1IE);
	TIM_ITConfig(TIM4, TIM_IT_CC1, DISABLE);

	//timer_enable_counter(TIM4);
	TIM_Cmd(TIM4, ENABLE);
}

void CLOCK_StartTimer(u16 us, timer_callback_t cb)
{
	if (!cb)
		return;
	timer_callback = cb;
	/* Counter enable. */
	u16 t = TIM_GetCounter(TIM4);
	/* Set the capture compare value for OC1. */
	TIM_SetCompare1(TIM4, us + t);

	TIM_ClearFlag(TIM4, TIM_FLAG_CC1);
	//timer_enable_irq(TIM4, TIM_DIER_CC1IE);
	TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);
}

void CLOCK_StopTimer()
{
	TIM_ITConfig(TIM4, TIM_IT_CC1, DISABLE);
	timer_callback = 0;
}
