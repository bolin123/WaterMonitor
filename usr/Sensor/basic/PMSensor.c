#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x0a
#define SENSOR_PM_INVALD_VALUE 99999.0

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
static uint32_t g_ackid;
//static uint8_t g_cmd;
static YMSensorType_t g_curSensor;

static float g_pm25Buffer[SENSOR_QUERY_CIRCLE];
static float g_pm10Buffer[SENSOR_QUERY_CIRCLE];

static uint8_t g_queryAckCount = 0;

static void pmSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

/*static void gotResult(void *args) //²âÊÔ½á¹û·µ»Ø
{
    int flag = (int)args;

    if(flag == 1)
    {
        SensorSetParamResult(true, g_ackid);
    }

    SensorGotResult(g_handle);
}*/

static float pmRealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_PM25 == type)
    {
        return g_pm25Buffer[g_queryAckCount - 1];
    }
    
    return g_pm10Buffer[g_queryAckCount - 1];
}

static void pmSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
{
    uint8_t i;
    union format_change argsData[11];

    SysLog("");
    for(i = 0; i < argsNum; i++)
    {
        SysPrintf("%f ", args[i]);
        argsData[i].f = args[i];
    }
    SysPrintf("\n");
    
    //todo: set args
    uint16_t startReg;
    if(type == YM_SENSOR_TYPE_PM25)
    {
        g_curSensor = YM_SENSOR_TYPE_PM25;
        startReg = 4;
    }
    else if(type == YM_SENSOR_TYPE_PM10)
    {
        g_curSensor = YM_SENSOR_TYPE_PM10;
        startReg = 8;        
    }
    g_ackid = ackid;
    SComModbusSet(g_device, 
                SCOM_MODBUS_CMD_SET_REG,
                startReg,
                argsNum*2,
                &argsData[0].b.byte1,
                argsNum*4);
    
}

static void pmSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
//    uint8_t i;
    SysLog("");
    
    //todo: set args
    uint16_t startReg;
    if(type == YM_SENSOR_TYPE_PM25)
    {
        g_curSensor = YM_SENSOR_TYPE_PM25;
        startReg = 4;
    }
    else if(type == YM_SENSOR_TYPE_PM10)
    {
        g_curSensor = YM_SENSOR_TYPE_PM10;
        startReg = 8;        
    }
    g_ackid = ackid;

    SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, startReg, 4);
}

static void pmSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("PM2.5:%.1f, PM10:%.1f\n", g_pm25Buffer[g_queryAckCount - 1], g_pm10Buffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 2);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("PM2.5-Rtd", SensorCalcAverage(g_pm25Buffer, g_queryAckCount), 0);
            SensorReportValue("PM10-Rtd", SensorCalcAverage(g_pm10Buffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_PM, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_PM, true);
        }
    }
}

static void pmSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("PM2.5-Rtd", SENSOR_PM_INVALD_VALUE, 0);//set default value
    SensorReportValue("PM10-Rtd", SENSOR_PM_INVALD_VALUE, 0);
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{
    //data[0] = cmd
    //data...
//    SysLog("");
    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x04))
    {
        SensorGotResult(g_handle);
        uint16_t value;
        value  = data[3]<<8;
        value |= data[2];
        g_pm25Buffer[g_queryAckCount] = (float)value/10.0;
        value  = data[5]<<8;
        value |= data[4];
        g_pm10Buffer[g_queryAckCount] = (float)value/10.0;
        g_queryAckCount++;
    }
    else if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x08))
    {
        union format_change argsData[2];
        memcpy(&argsData[0].b.byte1, &data[2], data[1]);
        
        if(g_curSensor == YM_SENSOR_TYPE_PM25)
        {
            SensorGetParamResult(YM_SENSOR_TYPE_PM25, 2, &argsData[0].f, 2, g_ackid);
            SensorGotResult(g_handle);
        }
        else if(g_curSensor == YM_SENSOR_TYPE_PM10)
        {
            SensorGetParamResult(YM_SENSOR_TYPE_PM10, 2, &argsData[0].f, 2, g_ackid);
            SensorGotResult(g_handle);            
        }
    }
    else if(data[0] == SCOM_MODBUS_CMD_SET_REG)
    {
        SensorSetParamResult(true, g_ackid);
        SensorGotResult(g_handle);
    }
}

static void pmSensorPoll(void)
{
}

static void pmSensorHalInit(void)
{
}

void PMSensorModuleInit(void)
{
    pmSensorHalInit();
    YMSensorType_t type[2] = {YM_SENSOR_TYPE_PM25, YM_SENSOR_TYPE_PM10};
    g_handle = SensorCreate(type, 2);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 50;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = pmSensorPoll;
        g_handle->prepare = pmSensorPrepare;
        g_handle->query   = pmSensorQuery;
        g_handle->realValue = pmRealValue;
        g_handle->getargs = pmSensorGetargs;
        g_handle->setargs = pmSensorSetargs;
        g_handle->power   = pmSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //pmSensorPowerctrl(true);
}

