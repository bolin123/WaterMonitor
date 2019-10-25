#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x08
#define SENSOR_NOISE_INVALD_VALUE 99999.0

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
//static uint32_t g_ackid;
//static uint8_t g_cmd;
//static YMSensorType_t g_curSensor;

static float g_noiseBuffer[SENSOR_QUERY_CIRCLE];


static uint8_t g_queryAckCount = 0;

static void noiseSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

static float noiseRealValue(YMSensorType_t type)
{
    return g_noiseBuffer[g_queryAckCount - 1];
}

static void noiseSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
{
    SysLog("");   
}

static void noiseSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
}

static void noiseSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("Noise:%.2f\n", g_noiseBuffer[g_queryAckCount - 1]);
    }

    g_queryCount++;

    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ2_REG, 0x0000, 1);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("NOISE-Rtd", SensorCalcAverage(g_noiseBuffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_NOISE, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_NOISE, true);
        }
    }
}

static void noiseSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("NOISE-Rtd", SENSOR_NOISE_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{
    if((data[0] == SCOM_MODBUS_CMD_READ2_REG)&&(data[1] == 0x02))
    {
        SensorGotResult(g_handle);
        uint16_t value;
        value  = data[2]<<8;
        value |= data[3];
        g_noiseBuffer[g_queryAckCount++] = (float)value/10.0; 
    }
    else
    {
    
    }
}

static void noiseSensorPoll(void)
{
}

static void noiseSensorHalInit(void)
{
}

void NoiseSensorModuleInit(void)
{
    noiseSensorHalInit();
    YMSensorType_t type = {YM_SENSOR_TYPE_NOISE};
    g_handle = SensorCreate(&type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 50;
        g_handle->preQueryWaitTime = 40;
        g_handle->poll    = noiseSensorPoll;
        g_handle->prepare = noiseSensorPrepare;
        g_handle->query   = noiseSensorQuery;
        g_handle->realValue = noiseRealValue;
        g_handle->getargs = noiseSensorGetargs;
        g_handle->setargs = noiseSensorSetargs;
        g_handle->power   = noiseSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //noiseSensorPowerctrl(true);
}

