#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x02
#define SENSOR_WD_INVALD_VALUE 99999.0

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
//static uint32_t g_ackid;
//static uint8_t g_cmd;
//static YMSensorType_t g_curSensor;

static float g_wdBuffer[SENSOR_QUERY_CIRCLE];


static uint8_t g_queryAckCount = 0;

static void wdSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

static void wdSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
{
    SysLog("");   
}

static void wdSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
}

static float wdRealValue(YMSensorType_t type)
{
    return g_wdBuffer[g_queryAckCount - 1];
}

static void wdSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("WD:%.1f\n", g_wdBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 1);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("WD-Rtd", SensorCalcAverage(g_wdBuffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_WD, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_WD, true);
        }
    }
}

static void wdSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WD-Rtd", SENSOR_WD_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{
//    SysLog("");
    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x02))
    {
        SensorGotResult(g_handle);
        uint16_t value;
        value  = data[2]<<8;
        value |= data[3];
        g_wdBuffer[g_queryAckCount++] = 360.0 - (float)value;
    }
    else
    {
    
    }
}

static void wdSensorPoll(void)
{
}

static void wdSensorHalInit(void)
{
}

void WDSensorModuleInit(void)
{
    wdSensorHalInit();
    YMSensorType_t type = {YM_SENSOR_TYPE_WD};
    g_handle = SensorCreate(&type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 500;
        g_handle->preQueryWaitTime = 400;
        g_handle->poll    = wdSensorPoll;
        g_handle->prepare = wdSensorPrepare;
        g_handle->query   = wdSensorQuery;
        g_handle->realValue = wdRealValue;
        g_handle->getargs = wdSensorGetargs;
        g_handle->setargs = wdSensorSetargs;
        g_handle->power   = wdSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //wdSensorPowerctrl(true);
}

