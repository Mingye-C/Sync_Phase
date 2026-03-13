#include "ad9959.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_usart.h"
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

void UART_Trans_Num(uint16_t num);

int main(void)
{ 
	uint16_t phase0 = 0;
	uint16_t mem=0;
	
    HAL_Init();                   	//初始化HAL库    
    Stm32_Clock_Init(336,8,2,7);  	//设置时钟,168Mhz
	delay_init(168);               	//初始化延时函数
	uart_init(115200);             	//初始化USART
	usmart_dev.init(168); 		    //初始化USMART
	LED_Init();						//初始化LED	
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
	KEY_Init();						//初始化KEY
	Init_AD9959();					//初始化DDS
	TIM2_Init(42-1, 4-1);		
	MY_ADC_Init();

	// delay_ms(2000);
	// HAL_Delay(2000);
	// HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);	

	// Write_Frequence(0, 5*MHz);
	AD9959_IO_Update();
	Synchronize_Frequence(0, 4, 1);
	AD9959_IO_Update();
	
	Write_Amplitude(0, 300);
	AD9959_IO_Update();
	Write_Phase(0, 0);
	AD9959_IO_Update();
	uint16_t data0 = Get_ADC();
	delay_ms(200);

	Write_Phase(0, 90);
	AD9959_IO_Update();
	uint16_t data1 = Get_ADC();
	delay_ms(200);

	Write_Phase(0, 180);
	AD9959_IO_Update();
	uint16_t data2 = Get_ADC();
	delay_ms(200);

	Write_Phase(0, 270);
	AD9959_IO_Update();
	uint16_t data3 = Get_ADC();
	delay_ms(200);

	UART_Trans_Num(data0);
	UART_Trans_Num(data1);
	UART_Trans_Num(data2);
	UART_Trans_Num(data3);

	uint16_t degree = 0;
	if (data0>1500) {
		degree = 0;
	}
	if (data1>1500) {
		degree = 90;
	}
	if (data2>1500) {
		degree = 180;
	}
	if (data3>1500) {
		degree = 270;
	}

	Write_Phase(0, degree);
	AD9959_IO_Update();

	while(1)
	{
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);	
		if (KEY_Scan(0)==KEY0) {
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);	
			degree=(degree+10)%360;
			Write_Phase(0, degree);
			AD9959_IO_Update();
			delay_ms(200);
		} else if (KEY_Scan(0)==KEY2) {
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);	
			degree=(degree-10)%360;
			Write_Phase(0, degree);
			AD9959_IO_Update();
			delay_ms(200);
		}

	}
}

void UART_Trans_Num(uint16_t num)
{
	uint8_t buffer[] = "0000\n";
	buffer[3]=num%10+'0';
	buffer[2]=(num/10)%10+'0';
	buffer[1]=(num/100)%10+'0';
	buffer[0]=(num/1000)%10+'0';
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_UART_Transmit(&UART1_Handler, buffer, 6, 200);
}

