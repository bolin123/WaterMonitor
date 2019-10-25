#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x02
#define SENSOR_COD_INVALD_VALUE 99999.0

#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0) 

typedef union 
{
    float f;
    unsigned char ch[4];
}IEEE754ToFloat_t;

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;

static float g_codBuffer[SENSOR_QUERY_CIRCLE];//COD
static uint8_t g_queryAckCount = 0;

static void codSensorPowerctrl(bool poweron)
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

static float codRealValue(YMSensorType_t type)
{
    return g_codBuffer[g_queryAckCount - 1];
}

static void codSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    if(type == YM_SENSOR_TYPE_COD)
    {
        HalFlashWriteInPage(HAL_ARGS_WT_COD_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
        SensorSetParamResult(true, ackid);
    }
}

static void codSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    if(type == YM_SENSOR_TYPE_COD)
    {
        HalFlashRead(HAL_ARGS_WT_COD_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        SensorGetParamResult(YM_SENSOR_TYPE_COD, 2, &argsData[0].f, 2, ackid);
    }    
}

static void codSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nCOD ==> %.3fmg/L\n", g_codBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0042, 0x02);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            float codValue = SensorCalcAverage(g_codBuffer, g_queryAckCount);
            SensorReportValue("WT-COD-Rtd", codValue, 0);
            SensorReportValue("WT-TOC-Rtd", codValue * 0.6, 0);
            SensorFaultReport(HAL_ERROR_CODE_COD, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_COD, true);
        }
    }
}

static void codSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-COD-Rtd", SENSOR_COD_INVALD_VALUE, 0);//set default value
    SensorReportValue("WT-TOC-Rtd", SENSOR_COD_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if(data[0] == SCOM_MODBUS_CMD_READ1_REG)
    {
        SensorGotResult(g_handle);
        union format_change argsData[2];        
        IEEE754ToFloat_t value;

        memcpy(value.ch, &data[2], 4);
        g_codBuffer[g_queryAckCount] = value.f;
        HalFlashRead(HAL_ARGS_WT_COD_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
        {
            g_codBuffer[g_queryAckCount] = argsData[0].f*g_codBuffer[g_queryAckCount]+argsData[1].f;        
        }
        
        g_queryAckCount++;
    }

}

static void codSensorPoll(void)
{
}

static void codSensorHalInit(void)
{
}

void CODSensorModuleInit(void)
{
    codSensorHalInit();
    YMSensorType_t type[2] = {YM_SENSOR_TYPE_COD, YM_SENSOR_TYPE_TOC};
    g_handle = SensorCreate(type, 2);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 150;
        g_handle->poll    = codSensorPoll;
        g_handle->prepare = codSensorPrepare;
        g_handle->query   = codSensorQuery;
        g_handle->realValue = codRealValue;
        g_handle->getargs = codSensorGetargs;
        g_handle->setargs = codSensorSetargs;
        g_handle->power   = codSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

