#include "Sensors.h"
#include "SensorCom.h"
#include "VTList.h"
#include "PowerManager.h"
#include "Manufacture.h"

#define SENSOR_CYCLE_QUERY_COUNT 10 //查询循环次数
#define SENSOR_QUERY_TIMEOUT     100 //查询超时时间100ms
#define SENSOR_QUERY_RETRY_NUM   1 //每次查询最大次数

typedef enum
{
    SENSOR_OPT_TYPE_QUERY = 0,
    SENSOR_OPT_TYPE_SET_PARAM,
    SENSOR_OPT_TYPE_GET_PARAM,
}SensorOptType_t;

typedef struct
{
    uint32_t ackid;
    YMSensorParam_t param;
}SensorOptArg_t;

typedef struct SensorProperty_st
{    
    bool poweron;
    bool queryDone;
    uint8_t *type;
    uint8_t typeNum;
    SensorHandle_t handle;
    VTLIST_ENTRY(struct SensorProperty_st);
}SensorProperty_t;

typedef struct SensorDataList_st
{   
    uint8_t retries;
    SysTime_t preActiveTime;
    SysTime_t activeTime;
    SensorOptType_t optType;
    SensorOptArg_t *optArgs;
    SensorHandle_t *handle;
    VTLIST_ENTRY(struct SensorDataList_st);
}SensorDataList_t;

static SensorProperty_t g_sensorProperty;
static SensorDataList_t g_sensorDataList;

static SysTime_t g_startTime;
static uint8_t g_registSensorNum = 0;
static uint8_t g_queryDoneNum = 0;

static PMDevice_t* g_pmDev;
static bool g_fallSleep = false;

static float g_avgValue[YM_SENSOR_TYPE_COUNT];

static void getAvgValue(const char *name, float value)
{
    uint8_t i;
    for(i = 0; i < YM_SENSOR_TYPE_COUNT; i++)
    {
        if(strstr(name, YMSensorName[YM_SENSOR_TYPE_COUNT-1-i]))
        {
            break;
        }
    }    
    if(i != YM_SENSOR_TYPE_COUNT)
    {
        g_avgValue[YM_SENSOR_TYPE_COUNT-1-i] = value;
    }
}

static SensorProperty_t *findSensorType(int type)
{
    uint8_t i;
    SensorProperty_t *sensor;

    VTListForeach(&g_sensorProperty, sensor)
    {
        for(i = 0; i < sensor->typeNum; i++)
        {
            if(sensor->type[i] == type)
            {
                return sensor;
            }
        }
    }

    return NULL;
}

float SensorCalcAverage(float val[], uint8_t num)
{
    uint8_t i;
    float count = 0.0, value;
    float min, max;

    min = val[0];
    max = val[0];
    for(i = 0; i < num; i++)
    {
        count += val[i];
        if(min > val[i])
        {
            min = val[i];
        }
        if(max < val[i])
        {
            max = val[i];
        }
    }
    
    if(num > 2)
    {

        value = (count - min - max) / (num - 2); //去掉最大和最小值取平均
    }
    else
    {
        value = count / num;
    }

    return value;
}

static SensorProperty_t *getNextSensor(SensorProperty_t *preSensor)
{
    SensorProperty_t *sensor = NULL;
    
    if(preSensor)
    {
        sensor = VTListFirst(preSensor);
        if(sensor == &g_sensorProperty)
        {
            //sensor = VTListFirst(&g_sensorProperty);
            sensor = NULL;
        }
    }
    else
    {
        sensor = VTListFirst(&g_sensorProperty);
    }

    return sensor;
}

static void dataListInsert(SensorHandle_t *handle, SensorOptType_t type, SensorOptArg_t *optArgs)
{
    SensorDataList_t *node;

    if(handle)
    {
        node = (SensorDataList_t *)SysMalloc(sizeof(SensorDataList_t));
        if(node)
        {
            node->handle        = handle;
            node->preActiveTime = SysTime();
            node->activeTime    = 0;
            node->retries       = 0;
            node->optType       = type;
            node->optArgs       = optArgs;
            VTListAdd(&g_sensorDataList, node);
        }
    }
}

static void dataListDel(SensorDataList_t *node)
{
    if(node)
    {
        VTListDel(node);
        if(node->optArgs)
        {
            if(node->optArgs->param.value)
            {
                
                free(node->optArgs->param.value);
            }
            
            free(node->optArgs);
        }
        free(node);
        node = NULL;
    }
}

void SensorQueryPollDone(SensorHandle_t *handle)
{
    SysLog("handle = %p", handle);
    if(handle)
    {
        ((SensorProperty_t *)handle->private)->queryDone = true;
        g_queryDoneNum++;
        SysPrintf("g_queryDoneNum = %d; g_registSensorNum = %d\n", g_queryDoneNum, g_registSensorNum);
    }
}

void SensorGotResult(SensorHandle_t *handle)
{
    SensorDataList_t *node = VTListFirst(&g_sensorDataList);

    if(node && node->handle == handle)
    {
        dataListDel(node);
    }
}

void SensorPowerStatus(SensorHandle_t *handle, bool poweron)
{
    SysLog("handle = %p, power = %d", handle, poweron);

    if(handle)
    {
        ((SensorProperty_t *)handle->private)->poweron = poweron;
    }
}

void SensorReportValue(const char *name, float value, uint8_t flag)
{
    YMPropertySet(name, value, flag);
    getAvgValue(name, value);
}

void SensorRecordAvgValue()
{
    SysRecordAverageData(g_avgValue, YM_SENSOR_TYPE_COUNT);
}

float SensorGetRealValue(YMSensorType_t type)
{
    SensorProperty_t *sensor = findSensorType(type);

    if(sensor && sensor->handle.realValue)
    {
        return sensor->handle.realValue(type);
    }
    return 0.0;
}

void SensorGetParamResult(YMSensorType_t type, uint8_t method, float *value, uint8_t valnum, uint32_t ackid)
{
    YMSensorParam_t param;

    SysLog("");
    if(value)
    {
        param.method = method;
        param.target = type;
        param.value  = value;
        param.valnum = valnum;
        YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_SUCCESS, &param);
    }
    else
    {
        YMReply(ackid, YM_REPLY_TYPE_SENSOR_PARAM, YM_PROCESS_RESULT_FAILED, NULL);
    }
}

void SensorSetParamResult(bool success, uint32_t ackid)
{
    SysLog("");
    if(success)
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, NULL);
    }
    else
    {
        YMReply(ackid, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, NULL);
    }
}

void SensorGetParam(YMSensorParam_t *args, uint32_t ackid)
{
    SensorProperty_t *sensor;
    SensorOptArg_t *optArg;

    if(args)
    {
        sensor = findSensorType(args->target);
        if(sensor)
        {
            optArg = (SensorOptArg_t *)SysMalloc(sizeof(SensorOptArg_t));

            if(optArg)
            {
                optArg->ackid = ackid;
                optArg->param.method = args->method;
                optArg->param.target = args->target;
                optArg->param.valnum = 0;
                optArg->param.value = NULL;
                dataListInsert(&sensor->handle, SENSOR_OPT_TYPE_GET_PARAM, optArg);
            }
        }
        else
        {
            SysLog("%d not found!", args->target);
        }
    }

}

void SensorSetParam(YMSensorParam_t *args, uint32_t ackid)
{
    SensorProperty_t *sensor;
    SensorOptArg_t *optArg;
    
    if(args)
    {
        uint8_t i;
        SysLog("method = %d, target = %d, valnum = %d", args->method, args->target, args->valnum);
        for(i = 0; i < args->valnum; i++)
        {
            SysPrintf("%f ", args->value[i]);
        }
        SysPrintf("\n");
        
        sensor = findSensorType(args->target);
        if(sensor)
        {
            optArg = (SensorOptArg_t *)SysMalloc(sizeof(SensorOptArg_t));

            if(optArg)
            {
                optArg->ackid = ackid;
                optArg->param.method = args->method;
                optArg->param.target = args->target;
                optArg->param.valnum = args->valnum;
                optArg->param.value = (float *)SysMalloc(args->valnum * sizeof(float));
                memcpy(optArg->param.value, args->value, args->valnum * sizeof(float));
                dataListInsert(&sensor->handle, SENSOR_OPT_TYPE_SET_PARAM, optArg);
            }
        }
        else
        {
            SysLog("%d not found!", args->target);
        }
    }
}

void SensorFaultReport(uint8_t faultNum, bool set)
{
    YMFaultsNumSet(faultNum, set);
}

static void dataListPoll(void)
{
    SensorDataList_t *node = VTListFirst(&g_sensorDataList);
    static SensorHandle_t *lastHandle = NULL;
    SensorOptArg_t *opt;
    SensorProperty_t *property;

    if(node && node->handle)
    {
        if(node->handle != lastHandle) //取到新发送列表
        {
            node->preActiveTime = SysTime();
            node->activeTime = 0;
            lastHandle = node->handle;
        }
        if(SysTimeHasPast(node->preActiveTime, node->handle->preQueryWaitTime))
        {
            if(SysTimeHasPast(node->activeTime, node->handle->aftQueryWaitTime))
            {
                if(node->retries >= SENSOR_QUERY_RETRY_NUM)
                {
                    property = (SensorProperty_t *)node->handle->private;
                    for(uint8_t i = 0; i < property->typeNum; i++)
                    {
                        SysPrintf("%s ", YMSensorName[property->type[i]]);
                    }
                    SysPrintf("opt[%d] timeout!!!\n", node->optType);
                    dataListDel(node);
                }
                else
                {
                    node->retries++;
                    //SysLog("");
                    node->activeTime = SysTime();
                    switch(node->optType)
                    {
                        case SENSOR_OPT_TYPE_QUERY:
                            if(node->handle->query)
                            {
                                node->handle->query();
                            }
                            break;
                        case SENSOR_OPT_TYPE_SET_PARAM:
                            if(node->handle->setargs && node->optArgs)
                            {
                                opt = node->optArgs;
                                node->handle->setargs((YMSensorType_t)opt->param.target, 
                                    opt->param.method, 
                                    opt->param.value, 
                                    opt->param.valnum,
                                    opt->ackid);
                            }
                            break;
                        case SENSOR_OPT_TYPE_GET_PARAM:
                            if(node->handle->getargs && node->optArgs)
                            {
                                opt = node->optArgs;
                                node->handle->getargs((YMSensorType_t)opt->param.target, 
                                    opt->param.method, 
                                    opt->ackid);
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        
    }
}

void SensorRegister(SensorHandle_t *handle)
{
    SensorProperty_t *node;
    uint8_t  i;
    
    if(handle && handle->private)
    {
        SysPrintf("SensorRegister: handle = %p, type:[", handle);
        node = (SensorProperty_t *)handle->private;
        for(i = 0; i < node->typeNum; i++)
        {
            SysPrintf("%s ", YMSensorName[node->type[i]]);
        }
        SysPrintf("]\n");
        VTListAdd(&g_sensorProperty, node);
        g_registSensorNum++;
    }
}

SensorHandle_t *SensorCreate(YMSensorType_t type[], uint8_t typeNum)
{
    uint8_t i;
    SensorProperty_t *sensor;

    sensor = (SensorProperty_t *)SysMalloc(sizeof(SensorProperty_t));
    if(sensor)
    {
        sensor->handle.private = sensor;
        sensor->poweron = false;
        sensor->typeNum = typeNum;
        sensor->type = (uint8_t *)SysMalloc(typeNum);
        if(sensor->type)
        {
            for(i = 0; i < typeNum; i++)
            {
                sensor->type[i] = type[i];
            }
        }
        
        return &sensor->handle;
    }
    return NULL;
}

#if 0
static void sensorComInit(void)
{
    HalUartConfig_t config;

    config.baudrate = 9600;
    config.flowControl = 0;
    config.parity = 0;
    config.wordLength = USART_WordLength_8b;
    config.recvCb = sensorDataRecv;
    HalUartConfig(SYS_SENSOR_COMM_PORT, &config);
}
#endif

static void sensorQueryPoll(void)
{    
    static SensorProperty_t *sensor = NULL;
    if(VTListFirst(&g_sensorDataList) == NULL)
    {
        sensor = getNextSensor(sensor);
        if(sensor == NULL)
        {
            //一轮结束
            if(SysTime() - g_startTime < 1900)
            {
                g_startTime += 1900;
            }
        }
        else
        {
            if(sensor->poweron && !sensor->queryDone)
            {
                dataListInsert(&sensor->handle, SENSOR_OPT_TYPE_QUERY, NULL);
            }
        }
    }
#if 0
    static bool first = true;

    uint8_t *ptr = NULL;
    uint8_t i;

    if(first)
    {
        for(i = 0; i < 100; i++)
        {
            ptr = SysMalloc(sizeof(SensorProperty_t));

            if(ptr)
            {
                SysPrintf("%p\n", ptr);
                free(ptr);
            }
        }

        ptr = SysMalloc(1024);
        if(ptr)
        {
            SysPrintf("%p\n", ptr);
            free(ptr);
        }
        first = false;
    }

#endif

}

static void sensorHandlePoll(void)
{
    SensorProperty_t *node;

    VTListForeach(&g_sensorProperty, node)
    {
        if(node->handle.poll)
        {
            node->handle.poll();
        }
    }
}

bool SensorIsQueryDone(void)
{
    return (g_queryDoneNum == g_registSensorNum);
}

void SensorStartQuery(SysTime_t delayTime)
{
    SensorProperty_t *sensor;

    SysLog("delay %d", delayTime);
  
    VTListForeach(&g_sensorProperty, sensor)
    {
        if(sensor->handle.prepare)
        {
            sensor->handle.prepare();
        }
        sensor->queryDone = false;
    }
    
    g_startTime = SysTime() + delayTime;
    g_queryDoneNum = 0;
}

static TestType_t sensor2TestType(SensorProperty_t *sensor)
{
#if defined(HAL_BOARD_NAME_LITE)
    switch(sensor->type[0])
    {
        case YM_SENSOR_TYPE_WP:
            return TEST_TYPE_WP;
        case YM_SENSOR_TYPE_TP:
            return TEST_TYPE_TH;
        case YM_SENSOR_TYPE_CO:
            return TEST_TYPE_I2C;
        case YM_SENSOR_TYPE_WD:
            return TEST_TYPE_ADC;
        default:
            return TEST_TYPE_NULL;
    }    
#else
    return TEST_TYPE_NULL;
#endif
}

void SensorSelfTest(void (*fn)(TestType_t, int8_t, void*))
{
    SensorProperty_t *sensor;
    int8_t res = 0;
    void *extra = NULL;
    VTListForeach(&g_sensorProperty, sensor)
    {
        if(sensor->handle.selfTest)
        {
            res = sensor->handle.selfTest(&extra);
            fn(sensor2TestType(sensor), res, extra);
        }
    }    
}

static void resume(bool rtcTimeup)
{
    SensorProperty_t *sensor;
    
    VTListForeach(&g_sensorProperty, sensor)
    {
        if(sensor->handle.power)
        {
            sensor->handle.power(true);
        }
    }
    g_fallSleep = false;
}

static int sleep(SysSleepMode_t mode)
{
    SensorProperty_t *sensor;
    
    VTListForeach(&g_sensorProperty, sensor)
    {
        if(sensor->handle.power)
        {
            sensor->handle.power(false);
        }
    }
    g_fallSleep = true;
    return 0;
}

static void sensorSleepHandle(void)
{
    SensorProperty_t *sensor;
    bool sleepDone = true;
    
    if(g_fallSleep)
    {
        VTListForeach(&g_sensorProperty, sensor)
        {
            if(sensor->poweron)
            {
                sleepDone = false;
                break;
            }
        }
        
        if(sleepDone)
        {
            PMDeviceSleepDone(g_pmDev);
            g_fallSleep = false;
        }
    }
}

static void pmInit(void)
{
    PMDevice_t device;

    device.resume = resume;
    device.sleep  = sleep;
    g_pmDev = PMRegiste(&device, false);
}

void SensorInitialize(void)
{
    VTListInit(&g_sensorProperty);
    VTListInit(&g_sensorDataList);
    pmInit();
    SComInitialize();
}

void SensorPoll(void)
{
    if(SysTime() > g_startTime)
    {
        sensorQueryPoll();
    }
    dataListPoll();
    sensorHandlePoll();
    sensorSleepHandle();
    SComPoll();
}

