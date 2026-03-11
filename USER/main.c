#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "usmart.h"
#include "touch.h"
#include <stdint.h>
#include <stdio.h>
#include "AD9959.h"
#include "adc.h"
#include "dac.h"
#include "timer.h"

#define MHz 1000000
#define kHz 1000

uint16_t adc_data[1024+32]={0};
uint8_t dma_finish_flag=0;


#define BACKGROUND WHITE
/************************************************
 ALIENTEK 探索者STM32F407开发板 实验28
 触摸屏实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

int main(void)
{ 
	uint16_t phase0 = 0;
	uint16_t mem=0;
	
    HAL_Init();                   	//初始化HAL库    
    Stm32_Clock_Init(336,8,2,7);  	//设置时钟,168Mhz
	delay_init(168);               	//初始化延时函数
	uart_init(115200);             	//初始化USART
	usmart_dev.init(84); 		    //初始化USMART
	LED_Init();						//初始化LED	
	KEY_Init();						//初始化KEY
	Init_AD9959();					//初始化DDS
	TIM2_Init(42-1, 4-1);		
	MY_ADC_Init();
		

	Write_Frequence(0, 5*MHz);
	AD9959_IO_Update();
	Synchronize_Frequence(1, 4, 1);
	AD9959_IO_Update();
	Write_Phase(1, 0*360/160);
	AD9959_IO_Update();
	Write_Amplitude(1, 300);
	AD9959_IO_Update();

	while(1)
	{
		printf("ADC:%u\n",Get_ADC());
	}
}

