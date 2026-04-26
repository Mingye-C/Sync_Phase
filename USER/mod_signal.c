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

// 新增全局变量用于频率追踪
float current_period_samples = 0; // 当前信号一个周期对应的采样点数
uint32_t sample_counter = 0;      // 采样计数器
uint16_t last_adc_val = 2048;     // 用于判断过零
// 在函数外定义一个静态变量用于平滑
static float smoothed_period = 0;

uint32_t ring_write_idx = 0;
uint32_t fixed_delay_samples = 29; // 固定延迟点数 (100kHz下，0点 = 0ms)

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
        uint16_t current_val = pIn[i];
        sample_counter++;

        // --- 1. 过零检测逻辑 (假设中位电压是 2048) ---
        // 检测上升沿过零点
        if (last_adc_val <= 2048 && current_val > 2048)
        {
            if (sample_counter > 5) // 简单防抖：频率不高于 20kHz
            {
                // --- 核心修改：增加一阶滤波，消除重影 ---
                if (smoothed_period == 0)
                    smoothed_period = sample_counter; // 初始化

                // 0.9f 表示 90% 保留旧值，10% 采用新值。这个比例越高，波形越稳，但频率跟踪越慢
                smoothed_period = (smoothed_period * 0.9f) + ((float)sample_counter * 0.1f);
                current_period_samples = smoothed_period;

                sample_counter = 0;
            }
        }
        last_adc_val = current_val;

        // --- 2. 存入环形缓冲区 ---
        sim_delay_ring_buf[ring_write_idx] = current_val;

        // --- 3. 线性插值跟随输出 ---
        float system_debt = 3.5f; // 硬件延迟通常不是整数个周期，可以带小数微调
        float precise_read_offset = current_period_samples - system_debt;

        if (precise_read_offset > 0)
        {
            float read_pos = (float)ring_write_idx - precise_read_offset;
            while (read_pos < 0)
                read_pos += DELAY_BUFFER_SIZE;

            // 取整数部分和小数部分
            int idx_low = (int)read_pos;
            int idx_high = (idx_low + 1) % DELAY_BUFFER_SIZE;
            float frac = read_pos - (float)idx_low;

            // 线性插值公式：y = y0 + frac * (y1 - y0)
            pOut[i] = (uint16_t)(sim_delay_ring_buf[idx_low] +
                                 frac * (sim_delay_ring_buf[idx_high] - sim_delay_ring_buf[idx_low]));
        }
        else
        {
            pOut[i] = current_val; // 频率极高或未检测到周期时，直接直通
        }

        // 更新索引
        ring_write_idx++;
        if (ring_write_idx >= DELAY_BUFFER_SIZE)
            ring_write_idx = 0;
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