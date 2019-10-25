#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x06
#define SENSOR_BOD_INVALD_VALUE 99999.0

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
static float g_tocBuffer[SENSOR_QUERY_CIRCLE];//TOC
static float g_bodBuffer[SENSOR_QUERY_CIRCLE];//BOD
static uint8_t g_queryAckCount = 0;

static void bodSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

static float bodRealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_BOD == type)
    {
        return g_bodBuffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_COD == type)
    {
        return g_codBuffer[g_queryAckCount - 1];
    }
    else
    {
        return g_tocBuffer[g_queryAckCount - 1];
    }
}

static void bodSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    if(type == YM_SENSOR_TYPE_BOD)
    {
        HalFlashWriteInPage(HAL_ARGS_WT_BOD_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
        SensorSetParamResult(true, ackid);
    }
}

static void bodSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    if(type == YM_SENSOR_TYPE_BOD)
    {
        HalFlashRead(HAL_ARGS_WT_BOD_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        SensorGetParamResult(YM_SENSOR_TYPE_BOD, 2, &argsData[0].f, 2, ackid);
    }    
}

static void bodSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nBOD ==> %.3f,COD = %.3f, TOC = %.3f\n", 
                                    g_bodBuffer[g_queryAckCount - 1],
                                    g_codBuffer[g_queryAckCount - 1],
                                    g_tocBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x004A, 0x08);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        //is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            float codValue = SensorCalcAverage(g_bodBuffer, g_queryAckCount);
            SensorReportValue("WT-CHRM-Rtd", codValue, 0);
            SensorFaultReport(HAL_ERROR_CODE_BOD, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_BOD, true);
        }
    }
}

static void bodSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-BOD-Rtd", SENSOR_BOD_INVALD_VALUE, 0);//set default value
    SensorReportValue("WT-COD-Rtd", SENSOR_BOD_INVALD_VALUE, 0);//set default value
    SensorReportValue("WT-TOC-Rtd", SENSOR_BOD_INVALD_VALUE, 0);//set default value
}

static float exchangeValue(uint8_t *data)
{
    IEEE754ToFloat_t value;

    value.ch[0] = data[3];
    value.ch[1] = data[2];
    value.ch[2] = data[1];        
    value.ch[3] = data[0];

    return value.f;
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if(data[0] == SCOM_MODBUS_CMD_READ1_REG)
    {
        SensorGotResult(g_handle);
        union format_change argsData[2];        
       
        g_tocBuffer[g_queryAckCount] = exchangeValue(&data[2]);
        g_codBuffer[g_queryAckCount] = exchangeValue(&data[10]);
        g_bodBuffer[g_queryAckCount] = exchangeValue(&data[14]);
        HalFlashRead(HAL_ARGS_WT_BOD_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
        {
            g_tocBuffer[g_queryAckCount] = argsData[0].f*g_tocBuffer[g_queryAckCount]+argsData[1].f; 
            g_codBuffer[g_queryAckCount] = argsData[0].f*g_codBuffer[g_queryAckCount]+argsData[1].f; 
            g_bodBuffer[g_queryAckCount] = argsData[0].f*g_bodBuffer[g_queryAckCount]+argsData[1].f; 
        }
        
        g_queryAckCount++;
    }

}

static void bodSensorPoll(void)
{
}

static void bodSensorHalInit(void)
{
}

void BODSensorModuleInit(void)
{
    bodSensorHalInit();
    YMSensorType_t type[3] = {YM_SENSOR_TYPE_BOD, YM_SENSOR_TYPE_TOC, YM_SENSOR_TYPE_COD};
    g_handle = SensorCreate(type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 10;
        g_handle->poll    = bodSensorPoll;
        g_handle->prepare = bodSensorPrepare;
        g_handle->query   = bodSensorQuery;
        g_handle->realValue = bodRealValue;
        g_handle->getargs = bodSensorGetargs;
        g_handle->setargs = bodSensorSetargs;
        g_handle->power   = bodSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

