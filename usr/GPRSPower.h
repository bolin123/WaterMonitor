#ifndef GPRS_POWER_H
#define GPRS_POWER_H

typedef enum
{
    GPRS_POWER_STATUS_OFF = 0,
    GPRS_POWER_STATUS_ON,
}GPRSPowerStatus_t;

GPRSPowerStatus_t GPRSPowerGetStatus(void);
void GPRSPowerReset(void);
void GPRSPowerOn(void);
void GPRSPowerOff(void);
void GPRSPowerInitialize(void);
void GPRSPowerPoll(void);

#endif
