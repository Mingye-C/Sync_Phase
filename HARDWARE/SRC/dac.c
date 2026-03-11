#include "dac.h"
#include "adc.h"

DAC_HandleTypeDef hdac1;
DMA_HandleTypeDef hdma1_dac1;
DAC_ChannelConfTypeDef DAC1_CH1;

extern uint8_t dma_finish_flag;

void MY_DAC_Init()
{
    __HAL_RCC_DAC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    hdac1.Instance=DAC1;

    hdma1_dac1.Instance=DMA1_Stream5;
    hdma1_dac1.Init.Channel=DMA_CHANNEL_7;
    hdma1_dac1.Init.Direction=DMA_MEMORY_TO_PERIPH;
    hdma1_dac1.Init.PeriphInc=DMA_PINC_DISABLE;
    hdma1_dac1.Init.MemInc=DMA_MINC_ENABLE;
    hdma1_dac1.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;
    hdma1_dac1.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;
    hdma1_dac1.Init.Mode=DMA_CIRCULAR;
    hdma1_dac1.Init.Priority=DMA_PRIORITY_HIGH;
    hdma1_dac1.Init.FIFOMode=DMA_FIFOMODE_DISABLE;
    hdma1_dac1.Init.FIFOThreshold=DMA_FIFO_THRESHOLD_HALFFULL;
    hdma1_dac1.Init.MemBurst=DMA_MBURST_SINGLE;
    hdma1_dac1.Init.PeriphBurst=DMA_PBURST_SINGLE;

    DAC1_CH1.DAC_Trigger=DAC_TRIGGER_T2_TRGO;
    DAC1_CH1.DAC_OutputBuffer=DAC_OUTPUTBUFFER_ENABLE;
    
    
    HAL_DMA_Init(&hdma1_dac1);
    __HAL_LINKDMA(&hdac1, DMA_Handle1, hdma1_dac1);

    HAL_DAC_ConfigChannel(&hdac1, &DAC1_CH1, DAC_CHANNEL_1);
    HAL_DAC_Init(&hdac1);
    
}

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
    if (hdac->Instance==DAC1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitTypeDef GPIO_InitStruct={0};
        GPIO_InitStruct.Pin=GPIO_PIN_4;
        GPIO_InitStruct.Mode=GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull=GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
