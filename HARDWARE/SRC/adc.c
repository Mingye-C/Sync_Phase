#include "adc.h"
#include "delay.h"
#include "dac.h"
#include "led.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_adc.h"
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//ADC驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/4/11
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

ADC_HandleTypeDef ADC1_Handler={0};//ADC句柄

//初始化ADC
//ch: ADC_channels 
//通道值 0~16取值范围为：ADC_CHANNEL_0~ADC_CHANNEL_16
void MY_ADC_Init(void)
{ 
    ADC1_Handler.Instance=ADC1;
    ADC1_Handler.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV2;   
    ADC1_Handler.Init.Resolution=ADC_RESOLUTION_12B;             //12位模式
    ADC1_Handler.Init.DataAlign=ADC_DATAALIGN_RIGHT;             //右对齐
    ADC1_Handler.Init.ScanConvMode=DISABLE;                      //非扫描模式
    ADC1_Handler.Init.EOCSelection=DISABLE;                      //关闭EOC中断
    ADC1_Handler.Init.ContinuousConvMode=DISABLE;                //关闭连续转换
    ADC1_Handler.Init.NbrOfConversion=1;                         //1个转换在规则序列中 也就是只转换规则序列1 
    ADC1_Handler.Init.DiscontinuousConvMode=DISABLE;             //禁止不连续采样模式
    ADC1_Handler.Init.NbrOfDiscConversion=0;                     //不连续采样通道数为0
    ADC1_Handler.Init.ExternalTrigConv=ADC_EXTERNALTRIGCONV_T2_TRGO;       //TIM2
    ADC1_Handler.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_RISING;//rising edge
    ADC1_Handler.Init.DMAContinuousRequests=DISABLE;             //DMA请求

    ADC_ChannelConfTypeDef chan = {0};
    chan.Channel = ADC_CHANNEL_13;
    chan.Rank = 1;
    chan.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&ADC1_Handler, &chan);

    HAL_ADC_Init(&ADC1_Handler);                                 //初始化 
    HAL_ADC_Start(&ADC1_Handler);
}

//ADC底层驱动，引脚配置，时钟使能
//此函数会被HAL_ADC_Init()调用
//hadc:ADC句柄
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_ADC1_CLK_ENABLE();            //使能ADC1时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();			//开启GPIOA时钟
	
    GPIO_Initure.Pin=GPIO_PIN_3;            //PC3
    GPIO_Initure.Mode=GPIO_MODE_ANALOG;     //模拟
    GPIO_Initure.Pull=GPIO_NOPULL;          //不带上下拉
    HAL_GPIO_Init(GPIOC,&GPIO_Initure);
}

u16 Get_ADC(void)
{
    HAL_ADC_PollForConversion(&ADC1_Handler, 0xffffffff);
    return HAL_ADC_GetValue(&ADC1_Handler);
}
