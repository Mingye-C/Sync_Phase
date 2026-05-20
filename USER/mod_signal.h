#ifndef __MOD_SIGNAL_H
#define __MOD_SIGNAL_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

// 1. 宏定义必须放在最前面，确保后面的数组声明能引用它们
#define SIM_BUFFER_SIZE 2
#define DELAY_BUFFER_SIZE 10000

// 2. 导出缓冲区（供 main.c 做快照分析）
extern uint16_t sim_adc_rx_buf[SIM_BUFFER_SIZE];
extern uint16_t sim_dac_tx_buf[SIM_BUFFER_SIZE];
extern uint32_t fixed_delay_samples;
extern uint8_t phase_shift_state;

// 3. 导出 DMA 句柄
extern DMA_HandleTypeDef hdma_adc2_sim;
extern DMA_HandleTypeDef hdma_dac1_ch2_sim;

// 4. 接口声明
void SIM_TIM2_TRGO_Init(void);
void SIM_ADC2_DMA_Init(void);
void SIM_DAC_DMA_Init(void);
void SIM_ModSignal_Start(void);
void SIM_Process_Block(uint16_t *pIn, uint16_t *pOut, uint16_t length);

#endif
