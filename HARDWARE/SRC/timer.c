#include "timer.h"
#include "led.h"
#include "main.h"
#include "adc.h"
#include <string.h>
#include "AD9959.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//定时器中断驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2017/4/7
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	


TIM_HandleTypeDef TIM3_Handler;      //定时器句柄 
TIM_HandleTypeDef htim2={0};

// uint16_t adc_data[1024]={0};
// uint16_t pos=0;

//通用定时器3中断初始化
//arr：自动重装值。
//psc：时钟预分频数
//定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
//Ft=定时器工作频率,单位:Mhz
//这里使用的是定时器3!(定时器3挂在APB1上，时钟为HCLK/2)
void TIM3_Init(u16 arr,u16 psc)
{  
    TIM3_Handler.Instance=TIM3;                          //通用定时器3
    TIM3_Handler.Init.Prescaler=psc;                     //分频系数
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //向上计数器
    TIM3_Handler.Init.Period=arr;                        //自动装载值
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//时钟分频因子
    HAL_TIM_Base_Init(&TIM3_Handler);

    TIM_MasterConfigTypeDef master = {0};
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;  // 更新事件触发ADC
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&TIM3_Handler, &master);
    
    //HAL_TIM_Base_Start(&TIM3_Handler); //使能定时器3
}

void TIM2_Init(u16 arr,u16 psc)
{  
    htim2.Instance=TIM2;                          
    htim2.Init.Prescaler=psc;                    
    htim2.Init.CounterMode=TIM_COUNTERMODE_UP;   
    htim2.Init.Period=arr;                       
    htim2.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);

    TIM_MasterConfigTypeDef master = {0};
    master.MasterOutputTrigger = TIM_TRGO_UPDATE;  
    master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &master);
}

//定时器底册驱动，开启时钟，设置中断优先级
//此函数会被HAL_TIM_Base_Init()函数调用
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(htim->Instance==TIM3)
	  {
		    __HAL_RCC_TIM3_CLK_ENABLE();            //使能TIM3时钟
		    // HAL_NVIC_SetPriority(TIM3_IRQn,1,3);    //设置中断优先级，抢占优先级1，子优先级3
		    // HAL_NVIC_EnableIRQ(TIM3_IRQn);          //开启ITM3中断   
	  }
    if (htim->Instance==TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    }
}



//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&TIM3_Handler);
}



//回调函数，定时器中断服务函数调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance==TIM3) {
        // if(pos%500==0) {LED0=!LED0;}
        // adc_data[pos]=Get_Adc(ADC_CHANNEL_0);
        // pos=(pos+1)%1024;
    }
}
