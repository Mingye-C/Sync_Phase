#ifndef __MOD_SIGNAL_H
#define __MOD_SIGNAL_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

extern DMA_HandleTypeDef hdma_adc2_sim;     // 供 stm32f4xx_it.c 调用
extern DMA_HandleTypeDef hdma_dac1_ch2_sim; // 供 stm32f4xx_it.c 调用

#define SIM_BUFFER_SIZE 1024
#define DELAY_BUFFER_SIZE 10000

extern uint32_t fixed_delay_samples;

// 外部接口声明
void SIM_TIM2_TRGO_Init(void);
void SIM_ADC2_DMA_Init(void);
void SIM_DAC_DMA_Init(void);
void SIM_ModSignal_Start(void); // 封装后的启动函数

// 处理函数声明
void SIM_Process_Block(uint16_t *pIn, uint16_t *pOut, uint16_t length);

#endif