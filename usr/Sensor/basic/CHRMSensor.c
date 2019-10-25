#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x05
#define SENSOR_CHRM_INVALD_VALUE 99999.0

#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0) 

typedef union 
{
    float f;
    unsigned char ch[4];
}IEEE754ToFloat_t;

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;

static float g_chrmBuffer[SENSOR_QUERY_CIRCLE];//COD
static uint8_t g_queryAckCount = 0;

static void chrmSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

static float chrmRealValue(YMSensorType_t type)
{
    return g_chrmBuffer[g_queryAckCount - 1];
}

static void chrmSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    if(type == YM_SENSOR_TYPE_CHRM)
    {
        HalFlashWriteInPage(HAL_ARGS_WT_CHRM_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
        SensorSetParamResult(true, ackid);
    }
}

static void chrmSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    if(type == YM_SENSOR_TYPE_CHRM)
    {
        HalFlashRead(HAL_ARGS_WT_CHRM_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        SensorGetParamResult(YM_SENSOR_TYPE_CHRM, 2, &argsData[0].f, 2, ackid);
    }    
}

static void chrmSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nCHRM ==> %.3f\n", g_chrmBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0186, 0x02);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        //is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            float codValue = SensorCalcAverage(g_chrmBuffer, g_queryAckCount);
            SensorReportValue("WT-CHRM-Rtd", codValue, 0);
            SensorFaultReport(HAL_ERROR_CODE_CHRM, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_CHRM, true);
        }
    }
}

static void chrmSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-CHRM-Rtd", SENSOR_CHRM_INVALD_VALUE, 0);//set default value
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if(data[0] == SCOM_MODBUS_CMD_READ1_REG)
    {
        SensorGotResult(g_handle);
        union format_change argsData[2];        
        IEEE754ToFloat_t value;

        uint8_t i = 0;
        //memcpy(value.ch, &data[2], 4);
        value.ch[i++] = data[5];
        value.ch[i++] = data[4];
        value.ch[i++] = data[3];        
        value.ch[i++] = data[2];
        g_chrmBuffer[g_queryAckCount] = value.f;
        HalFlashRead(HAL_ARGS_WT_CHRM_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
        {
            g_chrmBuffer[g_queryAckCount] = argsData[0].f*g_chrmBuffer[g_queryAckCount]+argsData[1].f;        
        }
        
        g_queryAckCount++;
    }

}

static void chrmSensorPoll(void)
{
}

static void chrmSensorHalInit(void)
{
}

void CHRMSensorModuleInit(void)
{
    chrmSensorHalInit();
    YMSensorType_t type[1] = {YM_SENSOR_TYPE_CHRM};
    g_handle = SensorCreate(type, 1);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 10;
        g_handle->poll    = chrmSensorPoll;
        g_handle->prepare = chrmSensorPrepare;
        g_handle->query   = chrmSensorQuery;
        g_handle->realValue = chrmRealValue;
        g_handle->getargs = chrmSensorGetargs;
        g_handle->setargs = chrmSensorSetargs;
        g_handle->power   = chrmSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

