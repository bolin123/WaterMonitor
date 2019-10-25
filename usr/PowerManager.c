#include "PowerManager.h"
#include "rtc.h"

#define PM_DEVICE_MAX_NUM 4

static PMDevice_t g_earlySleepDevice;
static PMDevice_t g_normalDevice[PM_DEVICE_MAX_NUM];
static uint8_t g_normalDevCount = 0;
static bool g_start = false;

static bool g_requestSleep = false;
static SysTime_t g_sleepTime;

void PMDeviceSleepDone(PMDevice_t *dev)
{
    if(dev && dev->enable)
    {
        SysLog("device %p sleep done", dev);
        dev->status = PM_STATUS_SLEEP_DONE;
    }
}

void PMDeviceWaittimeUpdate(SysTime_t time)
{
    if(SysTime() + time > g_sleepTime) //取最大延时时间
    {
        g_sleepTime = SysTime() + time;
    }
}

void PMDeviceRequestSleep(SysTime_t delay)
{
    SysLog("delay = %d", delay);
    if(g_start)
    {
        g_requestSleep = true;
        PMDeviceWaittimeUpdate(delay);
    }
}

void PMDeviceUnRequest(void)
{
     g_requestSleep = false;
}

static void wakeup(void)
{
    uint8_t i;
    bool isTimeup = SysIsRtcTimeup();

    SysSleepResume();

    for(i = 0; i < PM_DEVICE_MAX_NUM; i++)
    {
        if(g_normalDevice[i].enable)
        {
            g_normalDevice[i].status = PM_STATUS_WAKEUP;
            g_normalDevice[i].resume(isTimeup);
        }
    }
    
    if(g_earlySleepDevice.enable)
    {   
        g_earlySleepDevice.status = PM_STATUS_WAKEUP;
        g_earlySleepDevice.resume(isTimeup);
    }
    
}

void PMStart(void)//系统上电或standby唤醒时，执行
{
    g_start = true;
    SysRtcTimeup(true);
    wakeup();
}

void PMStop(void)
{
    g_start = false;
}

PMDevice_t *PMRegiste(PMDevice_t *dev, bool isEarly)
{
    if(isEarly)
    {
        if(!g_earlySleepDevice.enable)
        {
            g_earlySleepDevice = *dev;
            g_earlySleepDevice.enable = true;
            g_earlySleepDevice.status = PM_STATUS_WAKEUP;
            return &g_earlySleepDevice;
        }
    }
    else
    {
        if(g_normalDevCount < PM_DEVICE_MAX_NUM)
        {
            g_normalDevice[g_normalDevCount] = *dev;
            g_normalDevice[g_normalDevCount].enable = true;
            g_normalDevice[g_normalDevCount].status = PM_STATUS_WAKEUP;
            SysLog("%p", &g_normalDevice[g_normalDevCount]);
            return &g_normalDevice[g_normalDevCount++];
        }
    }
    
    return NULL;
}

static bool earlySleepDone(void)
{
    if(g_earlySleepDevice.enable)
    {
        return g_earlySleepDevice.status == PM_STATUS_SLEEP_DONE;
    }
    return true;
}

static bool normalSleepDone(void)
{
    uint8_t i;

    for(i = 0; i < g_normalDevCount; i++)
    {
        if(g_normalDevice[i].status != PM_STATUS_SLEEP_DONE)
        {
            return false;
        }
    }
    return true;
}

static void earlySleepHandle(void)
{
    if(g_earlySleepDevice.enable && g_earlySleepDevice.status == PM_STATUS_WAKEUP)
    {
        SysLog("%p", &g_earlySleepDevice);
        g_earlySleepDevice.status = PM_STATUS_START_SLEEP;
        g_earlySleepDevice.sleep(SysGetSleepMode());
    }
}

static void normalSleepHandle(void)
{
    uint8_t i;
    PMDevice_t *device;
    
    for(i = 0; i < g_normalDevCount; i++)
    {
        device = &g_normalDevice[i];
        if(device->enable)
        {
            if(device->status == PM_STATUS_WAKEUP)
            {
                SysLog("%p", device);
                device->status = PM_STATUS_START_SLEEP;
                device->sleep(SysGetSleepMode());
            }
        }
    }
}

static void doSleep(void)
{
    SysRtcTimeup(false); //clear rtc flag
    SysSleep();
    
    /*sleeping ...*/
    
    wakeup();
    SysSetDateTimeFromRtc();
    if(rtc_get_flag_IT_alr())
    {
        SysLog("!!!alarm on");
        rtc_alarm_set(rtc_get_counter() + SysGetReportInterval() - 1);
        rtc_clear_flag_IT_alr();
    }
}

static void fallSleep(void)
{
    if(g_requestSleep && SysTime() > g_sleepTime)
    {
        if(!earlySleepDone())
        {
            earlySleepHandle();
        }
        else
        {
            normalSleepHandle();
            if(normalSleepDone())
            {
                g_requestSleep = false;
                doSleep();
            }
        }
    }
}

void PMInitialize(void)
{
    g_normalDevCount = 0;
    memset(g_normalDevice, 0, sizeof(PMDevice_t) * PM_DEVICE_MAX_NUM);
}

void PMPoll(void)
{
    if(g_start)
    {
        fallSleep();

        if(rtc_get_flag_IT_alr())
        {
            SysLog("!!!alarm on");
            rtc_alarm_set(rtc_get_counter() + SysGetReportInterval() - 1);
            rtc_clear_flag_IT_alr();
            wakeup();
        }
    }
}

