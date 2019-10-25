#include "Sensors.h"
#include "SensorCom.h"
#include "math.h"

#define SSSENSOR_MODBUS_ADDR 0x03
#define SSSENSOR_QUERY_CIRCLE  30
#define SSSENSOR_ARGS_NAME "WT-SS-Rtd"

#define SENSOR_SS_INVALD_VALUE 99999.0
#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0) 

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;
static uint8_t g_queryAckCount = 0;
static float g_ssValue[SSSENSOR_QUERY_CIRCLE];

static void ssSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

static void ssSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    if(type == YM_SENSOR_TYPE_SS)
    {
        HalFlashWriteInPage(HAL_ARGS_WT_SS_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
        SensorSetParamResult(true, ackid);
    }
}


static void ssSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    if(type == YM_SENSOR_TYPE_SS)
    {
        HalFlashRead(HAL_ARGS_WT_SS_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        SensorGetParamResult(YM_SENSOR_TYPE_SS, 2, &argsData[0].f, 2, ackid);
    }    
}


static void ssSensorHalInit(void)
{
}

static float ssRealValue(YMSensorType_t type)
{
    return g_ssValue[g_queryAckCount - 1];
}

static void ssSensorPrepare(void)
{
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue(SSSENSOR_ARGS_NAME, SENSOR_SS_INVALD_VALUE, 0);//set default value
}

static void ssSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("SS ==> %.3fmg/L\n", g_ssValue[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SSSENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 0x04);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue(SSSENSOR_ARGS_NAME, SensorCalcAverage(g_ssValue, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_SS, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_SS, true);
        }
    }
}


static void ssSensorPoll(void)
{
}

static void modbusRecv(uint8_t *data, uint16_t len)
{
    uint16_t value;
    if(data[0] == SCOM_MODBUS_CMD_READ1_REG)
    {
        SensorGotResult(g_handle);
        if(data[1] == 0x08)
        {
            value = data[2];
            value = (value << 8) + data[3];
            union format_change argsData[2]; 
            HalFlashRead(HAL_ARGS_WT_TP_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            g_ssValue[g_queryAckCount] = (float)value / 10;
            if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            {
                g_ssValue[g_queryAckCount] = argsData[0].f*g_ssValue[g_queryAckCount]+argsData[1].f;
            }
            g_queryAckCount++;
        }
    }
}

/* Ðü¸¡Îï*/
void SSSensorModuleInit(void)
{
    ssSensorHalInit();
    YMSensorType_t type[1] = {YM_SENSOR_TYPE_SS};
    g_handle = SensorCreate(type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll      = ssSensorPoll;
        g_handle->prepare   = ssSensorPrepare;
        g_handle->query     = ssSensorQuery;
        g_handle->realValue = ssRealValue;
        g_handle->getargs   = ssSensorGetargs;
        g_handle->setargs   = ssSensorSetargs;
        g_handle->power     = ssSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SSSENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
}

