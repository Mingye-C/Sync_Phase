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

TIM_HandleTypeDef htim2_sim;
ADC_HandleTypeDef hadc2_sim;
DAC_HandleTypeDef hdac1_sim;
DMA_HandleTypeDef hdma_adc2_sim;
DMA_HandleTypeDef hdma_dac1_ch2_sim;
void SIM_TIM2_TRGO_Init(void);
void SIM_ADC2_DMA_Init(void);
void SIM_DAC_DMA_Init(void);

// 缓冲区定义 (各 1024 个半字，即 2048 字节)
#define SIM_BUFFER_SIZE 1024
uint16_t sim_adc_rx_buf[SIM_BUFFER_SIZE];
uint16_t sim_dac_tx_buf[SIM_BUFFER_SIZE];

// 延时环形缓冲区 (20KB, 建议在 MDK/CMake 链接脚本中定义到 CCMRAM，此处先放普通 RAM)
#define DELAY_BUFFER_SIZE 10000
uint16_t sim_delay_ring_buf[DELAY_BUFFER_SIZE];

uint16_t adc_data[1024 + 32] = {0};
uint8_t dma_finish_flag = 0;

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
	uint16_t mem = 0;

	HAL_Init();						// 初始化HAL库
	Stm32_Clock_Init(336, 8, 2, 7); // 设置时钟,168Mhz
	delay_init(168);				// 初始化延时函数
	uart_init(115200);				// 初始化USART
	usmart_dev.init(168);			// 初始化USMART
	LED_Init();						// 初始化LED
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);
	KEY_Init();	   // 初始化KEY
	Init_AD9959(); // 初始化DDS
	// TIM2_Init(42 - 1, 4 - 1);
	// MY_ADC_Init();

	// delay_ms(2000);
	// HAL_Delay(2000);
	// HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);

	SIM_TIM2_TRGO_Init();
	SIM_ADC2_DMA_Init();
	SIM_DAC_DMA_Init();
	// 2. 启动数据流 (注意顺序：先开输出DAC，再开输入ADC，最后开触发源TIM)
	HAL_DAC_Start_DMA(&hdac1_sim, DAC_CHANNEL_2, (uint32_t *)sim_dac_tx_buf, SIM_BUFFER_SIZE, DAC_ALIGN_12B_R);
	HAL_ADC_Start_DMA(&hadc2_sim, (uint32_t *)sim_adc_rx_buf, SIM_BUFFER_SIZE);
	HAL_TIM_Base_Start(&htim2_sim);

	// Write_Frequence(0, 5*MHz);
	AD9959_IO_Update();
	Synchronize_Frequence(0, 4, 4);
	AD9959_IO_Update();

	Write_Amplitude(0, 1000);
	AD9959_IO_Update();

	int max = 0, degree = 0;

	for (int i = 0; i < 360; i += 1)
	{
		Write_Phase(0, i);
		AD9959_IO_Update();
		uint16_t data = Get_ADC();
		if (data > max)
		{
			max = data;
			degree = i;
		}
		delay_ms(20);
	}

	Write_Phase(0, degree);
	AD9959_IO_Update();

	uint16_t phase10 = degree * 10;

	while (1)
	{
		HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
		if (KEY_Scan(0) == KEY0)
		{
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
			phase10 = (phase10 + 50) % 3600;
			Write_Phase(0, (float)phase10 / 10.0);
			AD9959_IO_Update();
			delay_ms(2000);
		}
		else if (KEY_Scan(0) == KEY2)
		{
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_SET);
			phase10 = (phase10 + 5) % 3600;
			Write_Phase(0, (float)phase10 / 10.0);
			AD9959_IO_Update();
			delay_ms(2000);
		}
	}
}

void UART_Trans_Num(uint16_t num)
{
	uint8_t buffer[] = "0000\n";
	buffer[3] = num % 10 + '0';
	buffer[2] = (num / 10) % 10 + '0';
	buffer[1] = (num / 100) % 10 + '0';
	buffer[0] = (num / 1000) % 10 + '0';
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10, GPIO_PIN_RESET);
	HAL_UART_Transmit(&UART1_Handler, buffer, 6, 200);
}

void SIM_TIM2_TRGO_Init(void)
{
	__HAL_RCC_TIM2_CLK_ENABLE();

	htim2_sim.Instance = TIM2;
	htim2_sim.Init.Prescaler = 84 - 1; // 84MHz / 84 = 1MHz
	htim2_sim.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2_sim.Init.Period = 10 - 1; // 1MHz / 10 = 100kHz
	htim2_sim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2_sim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	HAL_TIM_Base_Init(&htim2_sim);

	TIM_MasterConfigTypeDef sMasterConfig = {0};
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE; // 更新事件作为触发输出
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&htim2_sim, &sMasterConfig);
}

// B. ADC2 与 DMA2 初始化 (PA0)
void SIM_ADC2_DMA_Init(void)
{
	__HAL_RCC_ADC2_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_DMA2_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	hadc2_sim.Instance = ADC2;
	hadc2_sim.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc2_sim.Init.Resolution = ADC_RESOLUTION_12B;
	hadc2_sim.Init.ScanConvMode = DISABLE;
	hadc2_sim.Init.ContinuousConvMode = DISABLE;
	hadc2_sim.Init.DiscontinuousConvMode = DISABLE;
	hadc2_sim.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
	hadc2_sim.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO; // TIM2触发
	hadc2_sim.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc2_sim.Init.NbrOfConversion = 1;
	hadc2_sim.Init.DMAContinuousRequests = ENABLE;
	hadc2_sim.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	HAL_ADC_Init(&hadc2_sim);

	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
	HAL_ADC_ConfigChannel(&hadc2_sim, &sConfig);

	hdma_adc2_sim.Instance = DMA2_Stream2;
	hdma_adc2_sim.Init.Channel = DMA_CHANNEL_1;
	hdma_adc2_sim.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_adc2_sim.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_adc2_sim.Init.MemInc = DMA_MINC_ENABLE;
	hdma_adc2_sim.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_adc2_sim.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma_adc2_sim.Init.Mode = DMA_CIRCULAR;
	hdma_adc2_sim.Init.Priority = DMA_PRIORITY_HIGH;
	hdma_adc2_sim.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	HAL_DMA_Init(&hdma_adc2_sim);

	__HAL_LINKDMA(&hadc2_sim, DMA_Handle, hdma_adc2_sim);

	// 开启 ADC 接收对应的 DMA 中断
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

// C. DAC1_CH2 与 DMA1 初始化 (PA5)
void SIM_DAC_DMA_Init(void)
{
	__HAL_RCC_DAC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	hdac1_sim.Instance = DAC;
	HAL_DAC_Init(&hdac1_sim);

	DAC_ChannelConfTypeDef sConfig = {0};
	sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO; // TIM2触发
	sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	HAL_DAC_ConfigChannel(&hdac1_sim, &sConfig, DAC_CHANNEL_2);

	hdma_dac1_ch2_sim.Instance = DMA1_Stream6;
	hdma_dac1_ch2_sim.Init.Channel = DMA_CHANNEL_7;
	hdma_dac1_ch2_sim.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_dac1_ch2_sim.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_dac1_ch2_sim.Init.MemInc = DMA_MINC_ENABLE;
	hdma_dac1_ch2_sim.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_dac1_ch2_sim.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma_dac1_ch2_sim.Init.Mode = DMA_CIRCULAR;
	hdma_dac1_ch2_sim.Init.Priority = DMA_PRIORITY_MEDIUM;
	hdma_dac1_ch2_sim.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	HAL_DMA_Init(&hdma_dac1_ch2_sim);

	__HAL_LINKDMA(&hdac1_sim, DMA_Handle2, hdma_dac1_ch2_sim);
}

// ========================================================
// [新增区 5] 信道模拟器：核心 DSP 算法与中断回调
// ========================================================

// 环形时延缓冲区 (10000个点，最多支持 250ms 延迟)
#define DELAY_BUFFER_SIZE 10000
uint16_t sim_delay_ring_buf[DELAY_BUFFER_SIZE] = {0};
uint32_t ring_write_idx = 0;

// 固定延迟点数 (100kHz下，0点 = 0ms)
uint32_t fixed_delay_samples = 0;

// 核心数据处理函数 (在中断中被调用，每次处理 512 个半字)
void SIM_Process_Block(uint16_t *pIn, uint16_t *pOut, uint16_t length)
{
	// 建议此处添加 GPIO 翻转，用示波器测量 for 循环实际耗时
	// HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);

	for (int i = 0; i < length; i++)
	{
		// 1. 提取当前 ADC 采样值
		uint16_t current_sample = pIn[i];

		// 2. 将新样本推入环形仓库
		sim_delay_ring_buf[ring_write_idx] = current_sample;

		// 3. 计算历史数据的读取指针位置 (减去固定时延)
		int read_idx = ring_write_idx - fixed_delay_samples;
		if (read_idx < 0)
		{
			read_idx += DELAY_BUFFER_SIZE; // 指针回绕
		}

		// 4. 提取历史样本，装载至 DAC 缓冲区
		pOut[i] = sim_delay_ring_buf[read_idx];

		// 5. 更新写入指针
		ring_write_idx++;
		if (ring_write_idx >= DELAY_BUFFER_SIZE)
		{
			ring_write_idx = 0; // 指针回绕
		}
	}

	// HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET);
}

// --------------------------------------------------------
// HAL 库回调函数：当 DMA 传输完成一半时触发 (前半区满)
// --------------------------------------------------------
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC2)
	{
		// 处理前半区 512 个点
		SIM_Process_Block(&sim_adc_rx_buf[0], &sim_dac_tx_buf[0], SIM_BUFFER_SIZE / 2);
	}
}

// --------------------------------------------------------
// HAL 库回调函数：当 DMA 传输全部完成时触发 (后半区满)
// --------------------------------------------------------
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC2)
	{
		// 处理后半区 512 个点
		SIM_Process_Block(&sim_adc_rx_buf[SIM_BUFFER_SIZE / 2], &sim_dac_tx_buf[SIM_BUFFER_SIZE / 2], SIM_BUFFER_SIZE / 2);
	}
}
