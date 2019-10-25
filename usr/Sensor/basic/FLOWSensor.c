#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x04
#define SENSOR_FLOW_INVALD_VALUE 99999.0

#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0) 

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;

static float g_depBuffer[SENSOR_QUERY_CIRCLE];//深度
static float g_flowBuffer[SENSOR_QUERY_CIRCLE];//流速
static float g_tpBuffer[SENSOR_QUERY_CIRCLE];//水温

static uint8_t g_queryAckCount = 0;

static void flowSensorPowerctrl(bool poweron)
{
    SysLog("%d", poweron);
    //power contrl
    SensorPowerStatus(g_handle, poweron);
}

/*static void gotResult(void *args) //测试结果返回
{
    int flag = (int)args;

    if(flag == 1)
    {
        SensorSetParamResult(true, g_ackid);
    }

    SensorGotResult(g_handle);
}*/

static float flowRealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_COD == type)
    {
        return g_flowBuffer[g_queryAckCount - 1];
    }
    else
    {
        return g_tpBuffer[g_queryAckCount - 1];
    }
}

static void flowSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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

    switch(type)
    {
        case YM_SENSOR_TYPE_DEP:
            HalFlashWriteInPage(HAL_ARGS_WT_DEP_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_FLOW:
            HalFlashWriteInPage(HAL_ARGS_WT_FLOW_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        default:
            ;
    }
    SensorSetParamResult(true, ackid); 
}

static void flowSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    switch(type)
    {
        case YM_SENSOR_TYPE_DEP:
            HalFlashRead(HAL_ARGS_WT_DEP_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_DEP, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_FLOW:
            HalFlashRead(HAL_ARGS_WT_FLOW_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_FLOW, 2, &argsData[0].f, 2, ackid);
            break;
        default:
            ;
    }

}

static void flowSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nFLOW ==> depth:%.3fm, flow:%.3fm/s, tp:%.3f℃\n", g_depBuffer[g_queryAckCount - 1], g_flowBuffer[g_queryAckCount - 1], 
            g_tpBuffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 0x0e);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("WT-FLOW-Rtd", SensorCalcAverage(g_flowBuffer, g_queryAckCount), 0);
            SensorReportValue("WT-DEP-Rtd", SensorCalcAverage(g_depBuffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_FLOW, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_FLOW, true);
        }
    }
}

static void flowSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-FLOW-Rtd", SENSOR_FLOW_INVALD_VALUE, 0);//set default value
    SensorReportValue("WT-DEP-Rtd", SENSOR_FLOW_INVALD_VALUE, 0);
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x1c))
    {
        SensorGotResult(g_handle);
        union format_change argsData[2]; 
        uint32_t value;
        value  = data[2]<<24|data[3]<<16|data[4]<<8|data[5];
        g_depBuffer[g_queryAckCount] = *(float*)(&value);
        HalFlashRead(HAL_ARGS_WT_DEP_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_depBuffer[g_queryAckCount] = argsData[0].f*g_depBuffer[g_queryAckCount]+argsData[1].f;        
        value  = data[2+20]<<24|data[3+20]<<16|data[4+20]<<8|data[5+20];
        g_tpBuffer[g_queryAckCount] = *(float*)(&value);
        value  = data[2+24]<<24|data[3+24]<<16|data[4+24]<<8|data[5+24];
        g_flowBuffer[g_queryAckCount] = *(float*)(&value); 
        HalFlashRead(HAL_ARGS_WT_FLOW_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_flowBuffer[g_queryAckCount] = argsData[0].f*g_flowBuffer[g_queryAckCount]+argsData[1].f;         
        g_queryAckCount++;
    }
    else
    {
    
    }
}

static void flowSensorPoll(void)
{
}

static void flowSensorHalInit(void)
{
}

void FLOWSensorModuleInit(void)
{
    flowSensorHalInit();
    YMSensorType_t type[2] = {YM_SENSOR_TYPE_FLOW, YM_SENSOR_TYPE_DEP};
    g_handle = SensorCreate(type, 2);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = flowSensorPoll;
        g_handle->prepare = flowSensorPrepare;
        g_handle->query   = flowSensorQuery;
        g_handle->realValue = flowRealValue;
        g_handle->getargs = flowSensorGetargs;
        g_handle->setargs = flowSensorSetargs;
        g_handle->power   = flowSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

