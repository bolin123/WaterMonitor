#include "Yumair.h"

static void otaStatusProcess(YMOTAData_t *otaData)
{
    if(otaData)
    {
        switch(otaData->status)
        {
        case YM_OTA_STATUS_START:          //开始升级
            SysLog("start OTA...");
            break;
        case YM_OTA_STATUS_PROGRESS:       //升级进度
            SysLog("OTA progress:%d", otaData->progress);
            break;
        case YM_OTA_STATUS_RESULT_SUCCESS: //升级成功
            SysLog("OTA success!");
            if(otaData->info)
            {
                //TODO: SysOTAInfoSave(otaData->info);
            }
            SysReboot();
            break;
        case YM_OTA_STATUS_RESULT_FAILED:  //升级失败
            SysLog("OTA failed!");
            SysReboot();
            break;
        default:
            break;
        }
    }
}

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

static void yumairEventHandle(YMEvent_t event, void *args, unsigned int ackid)
{
    uint8_t i;
    bool noack = false;
    YMProcessResult_t result = YM_PROCESS_RESULT_SUCCESS;
    YMDateTime_t *date = (YMDateTime_t *)args;
    YMSensorParam_t *param = (YMSensorParam_t *)args;
    YMServerStatus_t *status = (YMServerStatus_t *)args;
    SysLocation_t *location;
    YMLocationInfo_t locationInfo;
    int value = (int)args;
    
    SysLog("event = %d", event);
    switch(event)
    {
    case YM_EVENT_REBOOT:          //重启
        SysReboot();
        break;
    case YM_EVENT_GET_SENSOR_ARGS:     //读取传感器值
        SysLog("method = %d, target = %d", param->method, param->target);
        switch(param->target) //YMSensorType_t
        {
            case YM_SENSOR_TYPE_PM25:
                break;
            case YM_SENSOR_TYPE_PM10:
                break;
            case YM_SENSOR_TYPE_CO:
                break;
            case YM_SENSOR_TYPE_NO2:
                break;
            case YM_SENSOR_TYPE_SO2:
                break;
            case YM_SENSOR_TYPE_O3:
                break;
            default:
                break;
        }
        //todo:get sensor params
        //if success
        //YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_SUCCESS, (void *)param);
        //else
        //YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_FAILED, NULL);
        
        noack = true;
        break;
    case YM_EVENT_SET_SENSOR_ARGS:     //设置传感器值
    
        SysLog("method = %d, target = %d", param->method, param->target);
        for(i = 0; i < param->valnum; i++)
        {
            SysPrintf("%f ", param->value[i]);
        }

        switch(param->target) //YMSensorType_t
        {
            case YM_SENSOR_TYPE_PM25:
                break;
            case YM_SENSOR_TYPE_PM10:
                break;
            case YM_SENSOR_TYPE_CO:
                break;
            case YM_SENSOR_TYPE_NO2:
                break;
            case YM_SENSOR_TYPE_SO2:
                break;
            case YM_SENSOR_TYPE_O3:
                break;
            default:
                break;
        }
        //todo:set sensor params
        //if success
        //YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, NULL);
        //else
        //YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, NULL);
        noack = true;
        break;
    case YM_EVENT_SET_POST_INTERVAL:   //设置上报时间间隔
        SysLog("interval = %d", value);
        //SysSetReportInterval(value);
        SysReboot(); //设置成功后重启生效
        break;
    case YM_EVENT_OTA_STATUS:          //OTA状态
        otaStatusProcess((YMOTAData_t *)args);
        break;
    case YM_EVENT_TIMMING:             //校时信息
        SysLog("Date Info: %d-%02d-%02d %02d:%02d:%02d.%03d", 
            date->year, 
            date->month, 
            date->day, 
            date->hour, 
            date->min, 
            date->sec,
            date->msec);
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
        noack = true;
        break;
    case YM_EVENT_SERVER_STATUS:       //服务器连接状态改变
        SysLog("Sever %d status:%d", status->sid, status->status);
        break;
    case YM_EVENT_SET_SLEEPMODE:
        SysLog("Sleep mode:%d", value);
        break;
    default:
        break;
    }

    if(ackid && !noack)
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, result, NULL);
    }
}

/*系统时钟
*1ms调用一次该函数
*/
void DemoDiDaTick(void)
{
    YM1msPast();
}

/*测试属性值上报*/
void demoPostPropertyValue(void)
{
    SysTime_t lastPostTime = 0;
    
    if(SysTimeHasPast(lastPostTime, 60000)) //60秒上报一次
    {
        YMPropertySet("WP-Rtd", 1.1, 1);
        YMPropertySet("TP-Rtd", 2.2, 1);
        YMPropertySet("TD-Rtd", 3.3, 1);
        YMPropertySet("WD-Rtd", 4.4, 1);
        YMPropertySet("WS-Rtd", 5.5, 1);
        YMPropertySet("PM2.5-Rtd", 6.6, 0);
        YMPropertySet("PM10-Rtd", 7.7, 0);
        YMPropertySet("CO-Rtd", 8.8, 0);
        YMPropertySet("NO2-Rtd", 9.9, 0);
        YMPropertySet("SO2-Rtd", 10.10, 0);
        YMPropertySet("O3-Rtd", 11.11, 0);
        
        YMPostAllProperties();
    }
}

/*异常上报*/
void DemoFaultReport(uint8_t faultNum, bool set) //上报异常码
{
    YMFaultsNumSet(faultNum, set);
}

/*校时*/
static void timingRequest(void)
{
    static uint8_t lastTimingHour = 0;
    struct YMDateTime_st * date = SysGetDateTime();

    if((date->hour % 3) == 0 && date->hour != lastTimingHour) // 3个小时校时一次
    {
        YMRequestTiming();
        lastTimingHour = date->hour;
    }
}

static void propertiesRegist(void)
{
    YMPropertyRegister("WP-Rtd",    "WP-Flag");
    YMPropertyRegister("TP-Rtd",    "TP-Flag");
    YMPropertyRegister("TD-Rtd",    "TD-Flag");
    YMPropertyRegister("WD-Rtd",    "WD-Flag");
    YMPropertyRegister("WS-Rtd",    "WS-Flag");
    YMPropertyRegister("PM2.5-Rtd", "PM2.5-Flag");
    YMPropertyRegister("PM10-Rtd",  "PM10-Flag");
    YMPropertyRegister("CO-Rtd",    "CO-Flag");
    YMPropertyRegister("NO2-Rtd",   "NO2-Flag");
    YMPropertyRegister("SO2-Rtd",   "SO2-Flag");
    YMPropertyRegister("O3-Rtd",    "O3-Flag");
}

void DemoInitialize(void)
{
    uint8_t version[4] = {1, 0, 0, 1};
    
    YMSetFirmVersion(version);
    YMSetDeviceModel("####$$$$");
    YMSetDevIDAndPwd("YMTEST0000000001", "12341234");
    YMSetReportInterval(60); //采集数据上报间隔 60S
    YMSetSleepMode(0); //休眠模式 用户自定义

    YMInitialize(yumairEventHandle); //初始化并注册事件处理函数
    propertiesRegist(); //注册属性
    YMStart();  //开始执行
}

void DemoPoll(void)
{
    YMPoll();
    timingRequest();
}


