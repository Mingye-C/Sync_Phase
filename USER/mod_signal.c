#include "mod_signal.h"

// ========================================================
// 1. 私有全局变量（不对 main.c 暴露，保证封装性）
// ========================================================
TIM_HandleTypeDef htim2_sim;
ADC_HandleTypeDef hadc2_sim;
DAC_HandleTypeDef hdac1_sim;
DMA_HandleTypeDef hdma_adc2_sim;
DMA_HandleTypeDef hdma_dac1_ch2_sim;

// DMA 双缓冲与时延环形缓冲区
uint16_t sim_adc_rx_buf[SIM_BUFFER_SIZE];
uint16_t sim_dac_tx_buf[SIM_BUFFER_SIZE];
uint16_t sim_delay_ring_buf[DELAY_BUFFER_SIZE] = {0};

uint32_t ring_write_idx = 0;
uint32_t fixed_delay_samples = 0; // 固定延迟点数 (100kHz下，0点 = 0ms)

// ========================================================
// 2. 外设初始化函数
// ========================================================
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
// 3. 封装：一键启动流
// ========================================================
void SIM_ModSignal_Start(void)
{
    // 注意顺序：先开输出DAC，再开输入ADC，最后开触发源TIM
    HAL_DAC_Start_DMA(&hdac1_sim, DAC_CHANNEL_2, (uint32_t *)sim_dac_tx_buf, SIM_BUFFER_SIZE, DAC_ALIGN_12B_R);
    HAL_ADC_Start_DMA(&hadc2_sim, (uint32_t *)sim_adc_rx_buf, SIM_BUFFER_SIZE);
    HAL_TIM_Base_Start(&htim2_sim);
}

// ========================================================
// 4. DSP 算法与中断回调
// ========================================================
void SIM_Process_Block(uint16_t *pIn, uint16_t *pOut, uint16_t length)
{
    for (int i = 0; i < length; i++)
    {
        uint16_t current_sample = pIn[i];
        sim_delay_ring_buf[ring_write_idx] = current_sample;

        int read_idx = ring_write_idx - fixed_delay_samples;
        if (read_idx < 0)
        {
            read_idx += DELAY_BUFFER_SIZE;
        }

        pOut[i] = sim_delay_ring_buf[read_idx];

        ring_write_idx++;
        if (ring_write_idx >= DELAY_BUFFER_SIZE)
        {
            ring_write_idx = 0;
        }
    }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC2)
    {
        SIM_Process_Block(&sim_adc_rx_buf[0], &sim_dac_tx_buf[0], SIM_BUFFER_SIZE / 2);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC2)
    {
        SIM_Process_Block(&sim_adc_rx_buf[SIM_BUFFER_SIZE / 2], &sim_dac_tx_buf[SIM_BUFFER_SIZE / 2], SIM_BUFFER_SIZE / 2);
    }
}