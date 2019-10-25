#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x03
#define SENSOR_TH_INVALD_VALUE 99999.0

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
//static uint32_t g_ackid;
//static uint8_t g_cmd;
//static YMSensorType_t g_curSensor;

static float g_wpBuffer[SENSOR_QUERY_CIRCLE];
static float g_tpBuffer[SENSOR_QUERY_CIRCLE];
static float g_tdBuffer[SENSOR_QUERY_CIRCLE];

static uint8_t g_queryAckCount = 0;

static void thSensorPowerctrl(bool poweron)
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

static float thRealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_WP == type)
    {
        return g_wpBuffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_TP == type)
    {
        return g_tpBuffer[g_queryAckCount - 1];
    }
    else
    {
        return g_tdBuffer[g_queryAckCount - 1];
    }
}

static void thSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
{
    SysLog("");   
}

static void thSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
}

static void thSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("WP:%.1f, TP:%.1f, TD:%.1f\n", g_wpBuffer[g_queryAckCount - 1], 
            g_tpBuffer[g_queryAckCount - 1], g_tdBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 3);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("WP-Rtd", SensorCalcAverage(g_wpBuffer, g_queryAckCount), 0);
            SensorReportValue("TP-Rtd", SensorCalcAverage(g_tpBuffer, g_queryAckCount), 0);
            SensorReportValue("TD-Rtd", SensorCalcAverage(g_tdBuffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_TH, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_TH, true);
        }
    }
}

static void thSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WP-Rtd", SENSOR_TH_INVALD_VALUE, 0);//set default value
    SensorReportValue("TP-Rtd", SENSOR_TH_INVALD_VALUE, 0);
    SensorReportValue("TD-Rtd", SENSOR_TH_INVALD_VALUE, 0);
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x06))
    {
        SensorGotResult(g_handle);
        uint16_t value;
        int16_t svalue;
        value  = data[2]<<8;
        value |= data[3];
        g_wpBuffer[g_queryAckCount] = (float)value/10.0;
        value  = data[4]<<8;
        value |= data[5];
        svalue = (int16_t)value;
        g_tpBuffer[g_queryAckCount] = (float)svalue/10.0;
        value  = data[6]<<8;
        value |= data[7];
        g_tdBuffer[g_queryAckCount] = (float)value/10.0; 
        g_queryAckCount++;
    }
    else
    {
    
    }
}

static void thSensorPoll(void)
{
}

static void thSensorHalInit(void)
{
}

void THSensorModuleInit(void)
{
    thSensorHalInit();
    YMSensorType_t type[3] = {YM_SENSOR_TYPE_WP, YM_SENSOR_TYPE_TP, YM_SENSOR_TYPE_TD};
    g_handle = SensorCreate(type, 3);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 50;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = thSensorPoll;
        g_handle->prepare = thSensorPrepare;
        g_handle->query   = thSensorQuery;
        g_handle->realValue = thRealValue;
        g_handle->getargs = thSensorGetargs;
        g_handle->setargs = thSensorSetargs;
        g_handle->power   = thSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

