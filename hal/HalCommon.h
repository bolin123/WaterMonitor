#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include "HalCtype.h"
#include "HalUart.h"
#include "HalGPIO.h"
#include "HalTimer.h"
#include "HalFlash.h"
#include "HalADC.h"
#include "HalWatchdog.h"

typedef enum
{
    HAL_BOOT_MODE_NORMAL = 0,
    HAL_BOOT_MODE_RTC,
}HalBootMode_t;

HalBootMode_t HalCommonBootMode(void);
void HalCommonStatusLedSet(uint8_t blink);
uint32_t HalGetSysTimeCount(void);
void HalInterruptSet(bool enable);
void HalTimerPast1ms(void);
void HalCommonReboot(void);
void HalCommonStandBy(void);
void HalCommonSleep(void);
void HalCommonResume(void);
void HalCommonInitialize(void);
void HalCommonPoll(void);

#endif

