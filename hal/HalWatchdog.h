#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

#include "HalCtype.h"

void HalWatchdogDisable(void);
void HalWatchdogFeed(void);
void HalWatchdogInitialize(void);
void HalWatchdogPoll(void);

#endif

