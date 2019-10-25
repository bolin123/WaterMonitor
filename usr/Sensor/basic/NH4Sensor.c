#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x0e
#define SENSOR_NH4_INVALD_VALUE 99999.0

#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0) 

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;

static float g_nh4Buffer[SENSOR_QUERY_CIRCLE];//COD

static uint8_t g_queryAckCount = 0;

static void nh4SensorPowerctrl(bool poweron)
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

static float nh4RealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_NH4 == type)
    {
        return g_nh4Buffer[g_queryAckCount - 1];
    }
    else
    {
        return 0;
    }
}

static void nh4SensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    if(type == YM_SENSOR_TYPE_NH4)
    {
        HalFlashWriteInPage(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
        SensorSetParamResult(true, ackid);
    } 
}

static void nh4SensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    if(type == YM_SENSOR_TYPE_NH4)
    {
        HalFlashRead(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        SensorGetParamResult(YM_SENSOR_TYPE_NH4, 2, &argsData[0].f, 2, ackid);
    }    
}

static void nh4SensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nNH4+ ==> NH4+: %.3fmg/L\n", g_nh4Buffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ2_REG, 0x0006, 0x01);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("WT-NH4-Rtd", SensorCalcAverage(g_nh4Buffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_NH4, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_NH4, true);
        }
    }
}

static void nh4SensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-NH4-Rtd", SENSOR_NH4_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if(data[0] == SCOM_MODBUS_CMD_READ2_REG)
    {
        SensorGotResult(g_handle);
        if(data[1] == 2)
        {
            int16_t sv;
            sv = data[2]<<8|data[3];
            g_nh4Buffer[g_queryAckCount] = (float)sv/10.0;
        }
        else if(data[1] == 3)
        {
            SysPrintf("NH4+ high\n");
        }
        union format_change argsData[2];
        HalFlashRead(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_nh4Buffer[g_queryAckCount] = argsData[0].f*g_nh4Buffer[g_queryAckCount]+argsData[1].f;

        g_queryAckCount++;
    }
    else
    {
    
    }
}

static void nh4SensorPoll(void)
{
}

static void nh4SensorHalInit(void)
{
}

void NH4SensorModuleInit(void)
{
    nh4SensorHalInit();
    YMSensorType_t type[1] = {YM_SENSOR_TYPE_NH4};
    g_handle = SensorCreate(type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 800;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = nh4SensorPoll;
        g_handle->prepare = nh4SensorPrepare;
        g_handle->query   = nh4SensorQuery;
        g_handle->realValue = nh4RealValue;
        g_handle->getargs = nh4SensorGetargs;
        g_handle->setargs = nh4SensorSetargs;
        g_handle->power   = nh4SensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

