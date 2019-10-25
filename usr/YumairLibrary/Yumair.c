#include "Yumair.h"
#include "YMCtype.h"
#include "YumairServer.h"
#include "YMPropertyManager.h"

static YmUint8_t g_version[YM_FIRMWARE_VERSION_LEN] = { 0 };
static char g_model[YM_DEVICE_MODEL_STRLEN + 1] = "";
static volatile YmUint32_t g_sysTime = 1;
static char g_password[YM_DEVICE_PASSWD_LEN + 1] = "";
static char g_id[YM_DEVICE_ID_LEN + 1] = "";
static YmUint16_t g_reportInterval;
static YmUint8_t g_sleepMode = 0;
static YMDateTime_t g_dateTime;

static YmUint64_t g_currentFaultNum = 0;
static YmUint64_t g_lastFaultNum = 0;

static YMEventCallback_t g_eventCallback = YmNULL;

_funcTag const char *YMGetDevPasswd(void)
{
    return g_password;
}

_funcTag const char *YMGetDevID(void)
{
    return g_id;
}

_funcTag YMDateTime_t *YMGetSysDateTime(void)
{
    return &g_dateTime;
}

_funcTag YmUint16_t YMGetReportInterval(void)
{
    return g_reportInterval;
}

_funcTag YmUint8_t YMGetSleepMode(void)
{
    return g_sleepMode;
}

#ifdef YM_USE_YUMAIR_DATE_TIME
_funcTag static YmBool isLeapYear(YmUint16_t year)
{
    if(year && year % 4 == 0)
    {
        if(year % 100 == 0)
        {
            if(year % 400 != 0)
            {
                return YmFalse;
            }
        }
        return YmTrue;
    }

    return YmFalse;
}

_funcTag static void dateExchange(YmUint8_t day)
{
    YmUint8_t monthDays;
    YmBool isLeap = isLeapYear(g_dateTime.year);

    switch(g_dateTime.month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        monthDays = 31;
        break;

    case 2:
        monthDays = isLeap ? 29 : 28;
        break;
    default:
        monthDays = 30;
    }

    if(day > monthDays)
    {
        g_dateTime.month++;
        if(g_dateTime.month > 12)
        {
            g_dateTime.year++;
            g_dateTime.month = 1;
        }
        g_dateTime.day = 1;
    }
}

_funcTag static void dateTimeUpdate(void)
{
    static YmUint32_t lastUpdateTime;

    if(YMSysTimeHasPast(lastUpdateTime, 1000))
    {
        g_dateTime.sec++;
        if(g_dateTime.sec >= 60)
        {
            g_dateTime.sec = 0;
            g_dateTime.min++;
            if(g_dateTime.min >= 60)
            {
                g_dateTime.min = 0;
                g_dateTime.hour++;
                if(g_dateTime.hour >= 24)
                {
                    g_dateTime.hour = 0;
                    g_dateTime.day++;
                    if(g_dateTime.day >= 28)
                    {
                        dateExchange(g_dateTime.day);
                    }
                }
            }
        }
        lastUpdateTime = YMSysTime();
    }
    g_dateTime.msec = YMSysTime() - lastUpdateTime;
}
#endif
_funcTag void YMSetDateTime(YMDateTime_t *time)
{
    if(time)
    {
        g_dateTime = *time;
    }
}

_funcTag void YMSetDevIDAndPwd(const char * id, const char * pwd)
{
    YMLog("ID:%s, Password:%s", id, pwd);
    if(id)
    {
        strcpy(g_id, id);
    }
    if(pwd)
    {
        strcpy(g_password, pwd);
    }
}

_funcTag void YMSetReportInterval(YmUint16_t seconds)
{
    YMLog("sec:%d", seconds);
    g_reportInterval = seconds;
}
_funcTag void YMSetSleepMode(unsigned char mode)
{
    YMLog("mode:%d", mode);
    g_sleepMode = mode;
}

_funcTag int YMPropertySet(const char * name, float value, YmUint8_t flag)
{
    YMLog("%s = %f", name, value);
    return YMPMSetValue(name, value, flag);
}

_funcTag int YMPropertyRegister(const char * name, const char * flagName)
{
    YMLog("%s:%s", name, flagName);
    return YMPMRegister(name, flagName);
}

_funcTag int YMPostAllProperties(void)
{
    return YMServerPostAllProperties();
}

_funcTag int YMRequestTiming(void)
{
    return YMServerRequestTiming();
}

_funcTag void YMLocationInfoReport(float latitude, float longitude)
{
    YMServerLocationReport(latitude, longitude);
}

_funcTag void YMFaultsNumSet(YmUint8_t faultNum, YmBool set)
{
    YmUint8_t bitval;

    bitval = (g_currentFaultNum & ((YmUint64_t)0x1 << (faultNum - 1))) != 0;

    if(set != bitval)
    {
        if(set)
        {
            g_currentFaultNum |= ((YmUint64_t)0x1 << (faultNum - 1));
        }
        else
        {
            g_currentFaultNum &= ~((YmUint64_t)0x1 << (faultNum - 1));
        }
    }
}

_funcTag YmUint8_t YMGetFaultNum(YmUint8_t *fbuff, YmBool force)
{
    YmUint8_t i, count = 0;

    if(force || g_currentFaultNum != g_lastFaultNum)
    {
        for(i = 0; i < 64; i++)
        {
            if(g_currentFaultNum & ((YmUint64_t)0x1 << i))
            {
                fbuff[count++] = i + 1;
            }
        }

        g_lastFaultNum = g_currentFaultNum;
    }
    return count;
}

_funcTag void YMReply(YmUint32_t replyID, YMReplyType_t type, YMProcessResult_t result, void * args)
{
    YMServerReply(replyID, type, result, args);
}

_funcTag YmUint32_t YMSysTime(void)
{
    return g_sysTime;
}

_funcTag void YM1msPast(void)
{
    g_sysTime++;
}

_funcTag const YmUint8_t *YMGetFirmVersion(void)
{
    return (const YmUint8_t *)g_version;
}

_funcTag const char *YMGetDeviceModel(void)
{
    return (const char *)g_model;
}

_funcTag void YMSetFirmVersion(const YmUint8_t * version)
{
    if(version)
    {
        memcpy(g_version, version, YM_FIRMWARE_VERSION_LEN);
    }
}

_funcTag void YMSetDeviceModel(const char * model)
{
    YMLog("model :%s", model);
    if(model)
    {
        strncpy(g_model, model, YM_DEVICE_MODEL_STRLEN);
    }
}

_funcTag static void eventEmit(YMEvent_t event, void *args, YmUint32_t replyID)
{
    if(g_eventCallback)
    {
        g_eventCallback(event, args, replyID);
    }
}

_funcTag static void ymEnventCallback(YMServerEvent_t event, void *args, YmUint32_t replyID)
{
    YMLog("event = %d", event);
    switch(event)
    {
    case YMSERVER_EVENT_REBOOT_DEVICE:
        eventEmit(YM_EVENT_REBOOT, args, replyID);
        break;
    case YMSERVER_EVENT_GET_SENSOR_ARGS:
        eventEmit(YM_EVENT_GET_SENSOR_ARGS, args, replyID);
        break;
    case YMSERVER_EVENT_SET_SENSOR_ARGS:
        eventEmit(YM_EVENT_SET_SENSOR_ARGS, args, replyID);
        break;
    case YMSERVER_EVENT_SET_POST_INTERVAL:
        eventEmit(YM_EVENT_SET_POST_INTERVAL, args, replyID);
        break;
    case YMSERVER_EVENT_SET_SLEEP_MODE:
        eventEmit(YM_EVENT_SET_SLEEPMODE, args, replyID);
        break;
    case YMSERVER_EVENT_CONNECT_STATUS:
        eventEmit(YM_EVENT_SERVER_STATUS, args, replyID);
        break;
    case YMSERVER_EVENT_TIMMING_INFO:
        eventEmit(YM_EVENT_TIMMING, args, replyID);
        break;
    case YMSERVER_EVENT_GET_LOCATION:
        eventEmit(YM_EVENT_GET_LOCATION, args, replyID);
        break;
    case YMSERVER_EVENT_OTA_STATUS:
        eventEmit(YM_EVENT_OTA_STATUS, args, replyID);
        break;
    case YMSERVER_EVENT_LINK_ABNORMAL:
        eventEmit(YM_EVENT_LINK_ABNORMAL, args, replyID);
        break;
    default:
        break;
    }
}

_funcTag void YMStart(void)
{
    YMLog("");
    YMServerStart();
}

_funcTag void YMStop(void)
{
    YMLog("");
    YMServerStop();
}

_funcTag void YMInitialize(YMEventCallback_t eventHandle)
{
    YMPMInitialize();
    YMServerInitialize();
    YMServerEventHandleRegister(ymEnventCallback);
    g_eventCallback = eventHandle;
}

_funcTag void YMPoll(void)
{
    YMPMPoll();
#ifdef YM_USE_YUMAIR_DATE_TIME
    dateTimeUpdate();
#endif
    YMServerPoll();
}

