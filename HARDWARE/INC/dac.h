#ifndef __DAC_H
#define __DAC_H

#include "sys.h"

extern DAC_HandleTypeDef hdac1;
extern DMA_HandleTypeDef hdma1_dac1;
extern DAC_ChannelConfTypeDef DAC1_CH1;

void MY_DAC_Init(void);

#endif
