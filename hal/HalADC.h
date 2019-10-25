#ifndef HAL_ADC_H
#define HAL_ADC_H

#include "HalCtype.h"


void HalADCInitialize(void);
void HalADCPoll(void);

uint16_t HalGetADCValue(void);



#endif
