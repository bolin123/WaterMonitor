#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"

//#define SENSOR_PWRCTRL_PIN   0x4e 

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x01
#define SENSOR_WS_INVALD_VALUE 99999.0

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
//static uint32_t g_ackid;
//static uint8_t g_cmd;
//static YMSensorType_t g_curSensor;

static float g_wsBuffer[SENSOR_QUERY_CIRCLE];


static uint8_t g_queryAckCount = 0;

static void wsSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    #if 0
    //power contrl
    if(poweron)
		HalGPIOSetLevel(SENSOR_PWRCTRL_PIN, 1);
	else
		HalGPIOSetLevel(SENSOR_PWRCTRL_PIN, 0);
    #endif
    SensorPowerStatus(g_handle, poweron);
}

static float wsRealValue(YMSensorType_t type)
{
    return g_wsBuffer[g_queryAckCount - 1];
}

static void wsSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
{
    SysLog("");   
}

static void wsSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
}

static void wsSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("WS:%.2f\n", g_wsBuffer[g_queryAckCount - 1]);
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
            SensorReportValue("WS-Rtd", SensorCalcAverage(g_wsBuffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_WS, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_WS, true);
        }
    }
}

static void wsSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WS-Rtd", SENSOR_WS_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x02))
    {
        SensorGotResult(g_handle);
        uint16_t value;
        value  = data[2]<<8;
        value |= data[3];
        g_wsBuffer[g_queryAckCount++] = (float)value/10.0;   
    }
    else
    {
    
    }
}

static void wsSensorPoll(void)
{
}

static void wsSensorHalInit(void)
{
//    HalGPIOConfig(SENSOR_PWRCTRL_PIN, HAL_IO_OUTPUT);
//    HalGPIOSetLevel(SENSOR_PWRCTRL_PIN, 0);
}

void WSSensorModuleInit(void)
{
    wsSensorHalInit();
    YMSensorType_t type = {YM_SENSOR_TYPE_WS};
    g_handle = SensorCreate(&type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 50;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = wsSensorPoll;
        g_handle->prepare = wsSensorPrepare;
        g_handle->query   = wsSensorQuery;
        g_handle->realValue = wsRealValue;
        g_handle->getargs = wsSensorGetargs;
        g_handle->setargs = wsSensorSetargs;
        g_handle->power   = wsSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //wsSensorPowerctrl(true);
}

