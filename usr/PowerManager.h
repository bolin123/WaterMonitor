#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "Sys.h"

typedef enum
{
    PM_STATUS_WAKEUP = 0,
    PM_STATUS_START_SLEEP,
    PM_STATUS_SLEEP_DONE,
}PMStatus_t;

typedef struct PMDevice
{
    bool enable;
    PMStatus_t status;
    int (*sleep)(SysSleepMode_t mode);
    void (*resume)(bool rtcTimeup);
}PMDevice_t;

void PMDeviceWaittimeUpdate(SysTime_t time);
void PMDeviceSleepDone(PMDevice_t *dev);
void PMDeviceRequestSleep(SysTime_t delay);
void PMDeviceUnRequest(void);

PMDevice_t *PMRegiste(PMDevice_t *dev, bool isEarly);

void PMStart(void);
void PMStop(void);
void PMInitialize(void);
void PMPoll(void);
#endif

