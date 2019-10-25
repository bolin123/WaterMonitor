#include "GPRSPower.h"
#include "GPRS.h"
#include "Sys.h"

#define GPRS_POWER_SWITCH_TIME 8000

static GPRSPowerStatus_t g_powerStatus = GPRS_POWER_STATUS_OFF;
static GPRSPowerStatus_t g_powerSetStatus = GPRS_POWER_STATUS_OFF;
static SysTime_t g_lastSetTime;

static void gprsPowerAdjust(void)
{
    if(g_powerStatus != g_powerSetStatus && \
        SysTimeHasPast(g_lastSetTime, GPRS_POWER_SWITCH_TIME))
    {
        if(g_powerSetStatus == GPRS_POWER_STATUS_ON)
        {
            GPRSPowerOn();
        }
        else
        {
            GPRSPowerOff();
        }
    }
}

static void gpowerEventHandle(GPRSEvent_t event, void *args)
{   
    GPRSStatus_t status;
    
    switch(event)
    {
        case GEVENT_POWER_ON:   //GPRS…œµÁ
            g_powerStatus = GPRS_POWER_STATUS_ON;
            break;
        case GEVENT_POWER_OFF:  //GPRSµÙµÁ
            g_powerStatus = GPRS_POWER_STATUS_OFF;
            break;
        case GEVENT_GPRS_STATUS_CHANGED:
            status = (GPRSStatus_t)(int)args;
            if(status == GPRS_STATUS_GPRS_DONE)
            {
                SysStatusLedSet(1);
            }
            else if(status == GPRS_STATUS_GSM_DONE)
            {
                SysStatusLedSet(2);
            }
            else
            {
                SysStatusLedSet(3);
            }
            break;
        default:
            break;
    }
}

GPRSPowerStatus_t GPRSPowerGetStatus(void)
{
    return g_powerStatus;
}

void GPRSPowerReset(void)
{
    GPRSStop();
    g_powerSetStatus = GPRS_POWER_STATUS_ON;
    g_lastSetTime = SysTime();
}

void GPRSPowerOn(void)
{
    if(g_powerStatus == GPRS_POWER_STATUS_OFF)
    {
        GPRSStart();
        g_powerSetStatus = GPRS_POWER_STATUS_ON;
        g_lastSetTime = SysTime();
    }
}

void GPRSPowerOff(void)
{
    if(g_powerStatus == GPRS_POWER_STATUS_ON)
    {
        GPRSStop();
        g_powerSetStatus = GPRS_POWER_STATUS_OFF;
        g_lastSetTime = SysTime();
    }
}

void GPRSPowerInitialize(void)
{
    GPRSEventHandleRegister(gpowerEventHandle);
}

void GPRSPowerPoll(void)
{
    gprsPowerAdjust();
}

