#include "APP.h"
#include "Yumair.h"
#include "SysTimer.h"
#include "GNSS.h"
#include "GPRS.h"
#include "GPRSPower.h"
#include "rtc.h"
//#include "sensor_cal_average.h"
//#include "modbus_protocol.h"
//#include "pm_25_10.h"
#include "PowerManager.h"
#include "Sensors.h"

#define APP_SENSOR_QUERY_DELAY_TIME 30000

static bool g_waitGPRSPowerDown = false;
static PMDevice_t* g_pmDev;
//static struct YMDateTime_st g_lastReportDate;

static void delayReboot(void *args)
{
    SysReboot();
}

static void otaStatusProcess(YMOTAData_t *otaData)
{
    if(otaData)
    {
        switch(otaData->status)
        {
        case YM_OTA_STATUS_START:          //开始升级
            SysLog("start OTA...");
            GPRSConfigDmaRecv();
            break;
        case YM_OTA_STATUS_PROGRESS:       //升级进度
            SysLog("OTA progress:%d", otaData->progress);
            break;
        case YM_OTA_STATUS_RESULT_SUCCESS: //升级成功
            SysOTAInfoSave(otaData->info);
            SysTimerSet(delayReboot, 10000, 0, NULL);
            SysLog("OTA success!");
            break;
        case YM_OTA_STATUS_RESULT_FAILED:  //升级失败
            SysTimerSet(delayReboot, 10000, 0, NULL);
            SysLog("OTA failed!");
            break;
        default:
            break;
        }
    }
}
#if 0

static void sendParams(YMSensorParam_t *param, uint32_t ackid)
{
    if(param)
    {
        YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_SUCCESS, (void *)param);
    }
    else
    {
        YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_FAILED, NULL);
    }
}

static void sendResult(uint32_t ackid, bool success)
{
    if(success)
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, NULL);
    }
    else
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, NULL);
    }
}

static void setSensorArgsHandle(YMSensorParam_t *param, uint32_t ackid)
{
    union format_change calibParam[11];
    
    if(param)
    {
        SysPrintf("set sensor :method = %d, target = %d\n", param->method, param->target);
        
        SysPrintf("Value:[");
        for(uint8_t i = 0; i < param->valnum; i++)
        {
            SysPrintf("%0.4f,", param->value[i]);
            calibParam[i].f = param->value[i];
        }
        SysPrintf("]\n");
        
        if(2 == param->method)
        {
            if(YM_SENSOR_TYPE_VOC == param->target)
            {                   
                //modbus写参数
                modbus_callback_t mcb;
                mcb.cb = (modbus_cb)sendResult;
                mcb.arg1 = ackid;
                mcb.arg2 = NULL;
                modbus_write_register(param->target + 2, \
                                        MODBUS_CMD_WRITE_REGIST, \
                                        4, \
                                        param->valnum * 2, \
                                        &calibParam[0].b.byte1, \
                                        100, \
                                        &mcb);
            }
            else if(YM_SENSOR_TYPE_PM25 == param->target)//flash 记录参数
            {
                set_pm25_cal_args(&calibParam[0].f);
                YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, NULL);
            }
            else if(YM_SENSOR_TYPE_PM10 == param->target)
            {
                set_pm10_cal_args(&calibParam[0].f);
                YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, NULL);
            }
        }
        else if(1 == param->method)
        {
            //modbus写参数
            modbus_callback_t mcb;
            mcb.cb = (modbus_cb)sendResult;
            mcb.arg1 = ackid;
            mcb.arg2 = NULL;
            modbus_write_register(param->target + 2, \
                MODBUS_CMD_WRITE_REGIST, \
                4, \
                param->valnum * 2, \
                &calibParam[0].b.byte1, \
                100, \
                &mcb);
        }
    }
    else
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, NULL);
    }
}

static void readSensorArgsHandle(YMSensorParam_t *param, uint32_t ackid)
{
    float calibParam[2];
    YMSensorParam_t *paramSend = (YMSensorParam_t *)SysMalloc(sizeof(YMSensorParam_t));
    
    SysPrintf("get sensor :method = %d, target = %d\n", param->method, param->target);
    
    if(paramSend == NULL)
    {
        return;
    }
    
    if(2 == param->method)
    {
        //flash 读取参数
        if(YM_SENSOR_TYPE_VOC == param->target)
        {
            paramSend->method = param->method;
            paramSend->target = param->target;
            paramSend->valnum = 2;
            paramSend->value  = NULL;
            modbus_callback_t mcb;
            mcb.cb = (modbus_cb)sendParams;
            mcb.arg1 = ackid;
            mcb.arg2 = paramSend;
            modbus_read_register(param->target + 2, \
                MODBUS_CMD_READ_REGIST, \
                0x04, \
                paramSend->valnum * 2, \
                100, \
                &mcb);
        }
        else if(YM_SENSOR_TYPE_PM25 == param->target)
        {
            get_pm25_cal_args(calibParam);
            paramSend->method = param->method;
            paramSend->target = param->target;
            paramSend->valnum = 2;
            paramSend->value  = calibParam;
            sendParams(paramSend, ackid);
        }
        else if(YM_SENSOR_TYPE_PM10 == param->target)
        {
            get_pm10_cal_args(calibParam);
            paramSend->method = param->method;
            paramSend->target = param->target;
            paramSend->valnum = 2;
            paramSend->value  = calibParam;
            sendParams(paramSend, ackid);
        }
    }
    else if(1 == param->method)
    {
        paramSend->method = param->method;
        paramSend->target = param->target;
        paramSend->valnum = 11;
        paramSend->value  = NULL;
        modbus_callback_t mcb;
        mcb.cb = (modbus_cb)sendParams;
        mcb.arg1 = ackid;
        mcb.arg2 = paramSend;
        modbus_read_register(param->target + 2, \
            MODBUS_CMD_READ_REGIST, \
            0x04, \
            paramSend->valnum*2, \
            100, \
            &mcb);
    }
}
#endif
static void yumairEventHandle(YMEvent_t event, void *args, unsigned int ackid)
{
    bool noack = false;
    YMProcessResult_t result = YM_PROCESS_RESULT_SUCCESS;
    YMDateTime_t *date = (YMDateTime_t *)args;
    YMSensorParam_t *param = (YMSensorParam_t *)args;
    YMServerStatus_t *status = (YMServerStatus_t *)args;
    SysLocation_t *location;
    YMLocationInfo_t locationInfo;
    uint32_t oldDate, newDate;
    int value = (int)args;
    
    SysLog("event = %d", event);
    switch(event)
    {
    case YM_EVENT_REBOOT:          //重启
        SysTimerSet(delayReboot, 10000, 0, NULL);
        break;
    case YM_EVENT_GET_SENSOR_ARGS:     //读取传感器值
        //readSensorArgsHandle(param, ackid);
        SensorGetParam(param, ackid);
        noack = true;
        break;
    case YM_EVENT_SET_SENSOR_ARGS:     //设置传感器值
        //setSensorArgsHandle(param, ackid); 
        SensorSetParam(param, ackid);
        noack = true;
        break;
    case YM_EVENT_SET_POST_INTERVAL:   //设置上报时间间隔
        SysLog("interval = %d", value);
        SysSetReportInterval(value);
        SysTimerSet(delayReboot, 10000, 0, NULL);
        break;
    case YM_EVENT_OTA_STATUS:          //OTA状态
        otaStatusProcess((YMOTAData_t *)args);
        break;
    case YM_EVENT_TIMMING:             //校时信息
        oldDate = rtc_get_counter();
        SysSetDateTime(date);
        rtc_set(*SysGetDateTime());
        newDate = rtc_get_counter();
        if(SysNeedSleep())
        {
            rtc_alarm_set(newDate - oldDate + rtc_alarm_get());
        }
        break;
    case YM_EVENT_GET_LOCATION:        //读取位置信息
        location = SysGetLocation();
        if(location)
        {
            locationInfo.latitude  = location->latitude;
            locationInfo.longitude = location->longitude;
            YMReply(ackid, YM_REPLY_TYPE_LOCATION, YM_PROCESS_RESULT_SUCCESS, (void *)&locationInfo);
        }
        else
        {
            YMReply(ackid, YM_REPLY_TYPE_LOCATION, YM_PROCESS_RESULT_NO_DATA, NULL);
        }
        GNSSStart();
        PMDeviceWaittimeUpdate(180000);
        noack = true;
        break;
    case YM_EVENT_SERVER_STATUS:       //服务器连接状态改变
        SysLog("Sever %d status:%d", status->sid, status->status);
        //if(status->status == YM_SERVER_LINK_ABNORMAL)
        //{
        //    SysPrintf("YM_SERVER_LINK_ABNORMAL !!!\n");
        //    GPRSPowerReset();
        //}
        break;
    case YM_EVENT_SET_SLEEPMODE:
        SysLog("Sleep mode:%d", value);
        SysSetSleepMode((uint8_t)value);
        SysTimerSet(delayReboot, 10000, 0, NULL);
        break;
    case YM_EVENT_LINK_ABNORMAL:
        SysLog("YM_EVENT_LINK_ABNORMAL !");
        GPRSPowerReset();
        break;
    default:
        break;
    }

    if(ackid && !noack)
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, result, NULL);
    }
}

void APPDiDaTick(void)
{
    YM1msPast();
}

#define APP_TIME_REQUEST_INTERVAL (30*60000) //30分钟
static void timingRequest(void)
{
    static SysTime_t lastRequestTime = 0;
    static SysTime_t retryTime = 0;

    if(SysTimeHasPast(lastRequestTime, APP_TIME_REQUEST_INTERVAL))
    {
        if(SysTimeHasPast(retryTime, 20000)) //若失败，则20秒重新尝试一次
        {
            if(YMRequestTiming() == 0) 
            {
                lastRequestTime = SysTime();
            }
            else
            {
                retryTime = SysTime();
            }
        }
    }
}

static void gpsQuery(void)
{
    static bool gpsStarted = false;
    if(!gpsStarted \
        && SysBootMode() == HAL_BOOT_MODE_NORMAL \
        && GPRSPowerGetStatus() == GPRS_POWER_STATUS_ON)
    {
        GNSSStart();
        PMDeviceWaittimeUpdate(180000);
        gpsStarted = true;
    }
}

static void propertiesRegist(void)
{
    YMPropertyRegister("WT-TP-Rtd",   "WT-TP-Flag");
    YMPropertyRegister("WT-EC-Rtd",   "WT-EC-Flag");
    YMPropertyRegister("WT-PH-Rtd",   "WT-PH-Flag");
    YMPropertyRegister("WT-O2-Rtd",   "WT-O2-Flag");
    YMPropertyRegister("WT-TB-Rtd",   "WT-TB-Flag");
    YMPropertyRegister("WT-COD-Rtd",  "WT-COD-Flag");
    YMPropertyRegister("WT-TOC-Rtd",  "WT-TOC-Flag");
    YMPropertyRegister("WT-FLOW-Rtd", "WT-FLOW-Flag");
    YMPropertyRegister("WT-DEP-Rtd",  "WT-DEP-Flag");

#if defined(DEVICE_TYPE_YT201A)
    YMPropertyRegister("WT-NH4-Rtd",  "WT-NH4-Flag");
    YMPropertyRegister("WT-SS-Rtd",   "WT-SS-Flag");
    YMPropertyRegister("WT-CHRM-Rtd", "WT-CHRM-Flag");
#elif defined(DEVICE_TYPE_YT201B)
    YMPropertyRegister("WT-BOD-Rtd", "WT-BOD-Flag");
#endif
}

static void gnssEventHandle(GNSSEvent_t event, void *args)
{
    GNSSLocation_t *gnss = (GNSSLocation_t *)args;
    
    switch(event)
    {
    case GEVENT_GNSS_START:      //GPS开启
        break;
    case GEVENT_GNSS_STOP:       //GPS关闭
        break;
    case GEVENT_GNSS_FIXED:      //GPS锁定信息
        SysSetLocation(gnss->location.latitude, gnss->location.longitude);
        YMLocationInfoReport(gnss->location.latitude, gnss->location.longitude);
        GNSSStop();
        YMFaultsNumSet(HAL_ERROR_CODE_GPS, false);
        break;
    case GEVENT_GNSS_TIMEOUT:
        GNSSStop();
        YMFaultsNumSet(HAL_ERROR_CODE_GPS, true);
        break;
    }
}

static void sleepPoll(void)
{
    if(g_waitGPRSPowerDown && GPRSPowerGetStatus() == GPRS_POWER_STATUS_OFF)
    {
        PMDeviceSleepDone(g_pmDev);
        g_waitGPRSPowerDown = false;
    }
}

static void resume(bool rtcTimeup)
{
    if(!rtcTimeup)
    {
        PMDeviceRequestSleep(20000);
    }
    else
    {
        PMDeviceUnRequest();
        SensorStartQuery(APP_SENSOR_QUERY_DELAY_TIME);
    }
}

static int sleep(SysSleepMode_t mode)
{
    SysLog("mode = %d", mode);
    if(mode == SYS_SLEEP_MODE_DEEP)
    {
        YMStop();
        g_waitGPRSPowerDown = true;
    }
    else if(mode == SYS_SLEEP_MODE_LIGHT)
    {
        GPRSConfigIrqRecv();
        PMDeviceSleepDone(g_pmDev);
    }
    else
    {
    }
    return 0;
}
static void pmInit(void)
{
    PMDevice_t device;

    device.resume = resume;
    device.sleep  = sleep;
    g_pmDev = PMRegiste(&device, true);
}

static void valueReportPoll(void)
{
    static uint32_t lastUtcTime;
    if(SensorIsQueryDone() && SysGetDateTime()->year > 2017 && SysUtcTimePast(lastUtcTime, SysGetReportInterval()))
    {
        YMPostAllProperties();
        SensorRecordAvgValue();
        PMDeviceRequestSleep(20000); //request fall sleep
        //g_lastReportDate = *SysGetDateTime();
        lastUtcTime = SysUtcTime();

        if(SysGetSleepMode() == SYS_SLEEP_MODE_NONE)
        {
            SensorStartQuery(0);
        }
    }
}

static void sensorConfig(void)
{
#if defined(DEVICE_TYPE_YT201A)
    extern void MPSSensorModuleInit(void);
    extern void CODSensorModuleInit(void);
    extern void FLOWSensorModuleInit(void);
    extern void CHRMSensorModuleInit(void); 
    extern void SSSensorModuleInit(void);
    MPSSensorModuleInit();    //多参数（水温、pH、电导率、溶解氧、浊度、氨氮）
    CODSensorModuleInit();    //cod\toc
    FLOWSensorModuleInit();   //流量和水位
    CHRMSensorModuleInit();   //色度
    SSSensorModuleInit();     //悬浮物
#elif defined(DEVICE_TYPE_YT201B)
    extern void MPSSensorModuleInit(void);
    extern void FLOWSensorModuleInit(void);
    extern void BODSensorModuleInit(void);
    MPSSensorModuleInit();    //多参数（水温、pH、电导率、溶解氧、浊度、氨氮）
    FLOWSensorModuleInit();   //流量和水位
    BODSensorModuleInit();
#endif
    if(!SysNeedSleep())
    {
        SensorStartQuery(APP_SENSOR_QUERY_DELAY_TIME);
    }
}

void APPInitialize(void)
{
    SysSetDevType("$$$$####");
    
    YMSetFirmVersion(SysGetVersion());
    YMSetDeviceModel(SysGetDevType());
    YMSetDevIDAndPwd(SysGetDeviceID(), SysGetDevicePwd());
    YMSetReportInterval(SysGetReportInterval());
    YMSetSleepMode(SysGetSleepMode());

    YMInitialize(yumairEventHandle);
    GNSSEventHandleRegister(gnssEventHandle);

    SysSetSensorConfig();
    propertiesRegist();
    pmInit();
    YMStart();
    sensorConfig();
}

void APPPoll(void)
{
    YMPoll();
    timingRequest();
    gpsQuery();
    sleepPoll();
    valueReportPoll();
}

