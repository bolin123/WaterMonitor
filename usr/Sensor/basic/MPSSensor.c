#include "Sensors.h"
#include "SensorCom.h"
#include "SysTimer.h"
#include "math.h"

#define SENSOR_QUERY_CIRCLE  30

#define SENSOR_MODBUS_ADDR   0x01
#define SENSOR_MPS_INVALD_VALUE 99999.0

#define IS_PARAM_LEGAL(x)  (fabs(x)<1000.0)  

static uint8_t g_queryCount = 0;
static SensorHandle_t *g_handle = NULL;
static SComDevice_t *g_device;

/*五参数传感器*/
static float g_tpBuffer[SENSOR_QUERY_CIRCLE];//水温
static float g_ecBuffer[SENSOR_QUERY_CIRCLE];//电导率
static float g_phBuffer[SENSOR_QUERY_CIRCLE];//ph
static float g_o2Buffer[SENSOR_QUERY_CIRCLE];//溶解氧
static float g_tbBuffer[SENSOR_QUERY_CIRCLE];//浊度
static float g_nh4Buffer[SENSOR_QUERY_CIRCLE];//氨氮

static uint8_t g_queryAckCount = 0;

static void mpsSensorPowerctrl(bool poweron)
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

static float mpsRealValue(YMSensorType_t type)
{
    if(YM_SENSOR_TYPE_EC == type)
    {
        return g_ecBuffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_PH == type)
    {
        return g_phBuffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_O2 == type)
    {
        return g_o2Buffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_TB == type)
    {
        return g_tbBuffer[g_queryAckCount - 1];
    }
    else if(YM_SENSOR_TYPE_TP == type)
    {
        return g_tpBuffer[g_queryAckCount - 1];
    }
    else
    {
        return g_nh4Buffer[g_queryAckCount - 1];
    }
}

static void mpsSensorSetargs(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid)
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
        case YM_SENSOR_TYPE_TP:
            HalFlashWriteInPage(HAL_ARGS_WT_TP_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_EC:
            HalFlashWriteInPage(HAL_ARGS_WT_EC_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_PH:
            HalFlashWriteInPage(HAL_ARGS_WT_PH_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_O2:
            HalFlashWriteInPage(HAL_ARGS_WT_O2_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_TB:
            HalFlashWriteInPage(HAL_ARGS_WT_TB_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        case YM_SENSOR_TYPE_NH4:
            HalFlashWriteInPage(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, &argsData[0].b.byte1, 8);
            break;
        default:
            ;
    }
    SensorSetParamResult(true, ackid);

}

static void mpsSensorGetargs(YMSensorType_t type, uint8_t method, uint32_t ackid)
{
    SysLog("");
    union format_change argsData[11];
    switch(type)
    {
        case YM_SENSOR_TYPE_TP:
            HalFlashRead(HAL_ARGS_WT_TP_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_TP, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_EC:
            HalFlashRead(HAL_ARGS_WT_EC_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_EC, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_PH:
            HalFlashRead(HAL_ARGS_WT_PH_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_PH, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_O2:
            HalFlashRead(HAL_ARGS_WT_O2_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_O2, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_TB:
            HalFlashRead(HAL_ARGS_WT_TB_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_TB, 2, &argsData[0].f, 2, ackid);
            break;
        case YM_SENSOR_TYPE_NH4:
            HalFlashRead(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
            SensorGetParamResult(YM_SENSOR_TYPE_NH4, 2, &argsData[0].f, 2, ackid);
            break;
        default:
            ;
    }

}

static void mpsSensorQuery(void)
{
    if(g_queryAckCount > 0)
    {
        SysPrintf("\nMPS ==> EC:%.3fuS/cm, PH:%.3f, O2:%.3fmg/L, tb:%.3fNTU， tp:%.3f℃, nh4:%.3f\n", 
                    g_ecBuffer[g_queryAckCount - 1], 
                    g_phBuffer[g_queryAckCount - 1], 
                    g_o2Buffer[g_queryAckCount - 1], 
                    g_tbBuffer[g_queryAckCount - 1], 
                    g_tpBuffer[g_queryAckCount - 1],
                    g_nh4Buffer[g_queryAckCount - 1]);
    }
    g_queryCount++;
    if(g_queryCount <= SENSOR_QUERY_CIRCLE)
    {
        SComModbusRead(g_device, SCOM_MODBUS_CMD_READ1_REG, 0x0000, 0x12);
    }
    else
    {
        SensorQueryPollDone(g_handle);

        // todo:is value valid?
        if(g_queryAckCount > g_queryCount/2)
        {
            SensorReportValue("WT-TP-Rtd", SensorCalcAverage(g_tpBuffer, g_queryAckCount), 0);
            SensorReportValue("WT-EC-Rtd", SensorCalcAverage(g_ecBuffer, g_queryAckCount), 0);
            SensorReportValue("WT-PH-Rtd", SensorCalcAverage(g_phBuffer, g_queryAckCount), 0);
            SensorReportValue("WT-O2-Rtd", SensorCalcAverage(g_o2Buffer, g_queryAckCount), 0);
            SensorReportValue("WT-TB-Rtd", SensorCalcAverage(g_tbBuffer, g_queryAckCount), 0);
            SensorReportValue("WT-NH4-Rtd", SensorCalcAverage(g_nh4Buffer, g_queryAckCount), 0);
            SensorFaultReport(HAL_ERROR_CODE_MPS, false);
        }
        else
        {
            SensorFaultReport(HAL_ERROR_CODE_MPS, true);
        }
    }
}

static void mpsSensorPrepare(void)
{
    SysLog("");
    g_queryCount = 0;
    g_queryAckCount = 0;
    SensorReportValue("WT-TP-Rtd", SENSOR_MPS_INVALD_VALUE, 0);//set default value
    SensorReportValue("WT-EC-Rtd", SENSOR_MPS_INVALD_VALUE, 0);
    SensorReportValue("WT-PH-Rtd", SENSOR_MPS_INVALD_VALUE, 0);
    SensorReportValue("WT-O2-Rtd", SENSOR_MPS_INVALD_VALUE, 0);
    SensorReportValue("WT-TB-Rtd", SENSOR_MPS_INVALD_VALUE, 0);
    SensorReportValue("WT-NH4-Rtd", SENSOR_MPS_INVALD_VALUE, 0);
}

static void modbusRecv(uint8_t *data, uint16_t dlen)
{    
    if((data[0] == SCOM_MODBUS_CMD_READ1_REG)&&(data[1] == 0x24))
    {
        SensorGotResult(g_handle);
        union format_change argsData[2];
        int16_t svInt, svDec;
        svInt  = data[2]<<8;
        svInt |= data[3];
        svDec  = data[4]<<8;
        svDec |= data[5];
        g_tpBuffer[g_queryAckCount] = (float)svInt/pow(10, svDec);
        HalFlashRead(HAL_ARGS_WT_TP_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_tpBuffer[g_queryAckCount] = argsData[0].f*g_tpBuffer[g_queryAckCount]+argsData[1].f;
        svInt  = data[2+12]<<8;
        svInt |= data[3+12];
        svDec  = data[4+12]<<8;
        svDec |= data[5+12];
        g_ecBuffer[g_queryAckCount] = (float)svInt/pow(10, svDec);
        HalFlashRead(HAL_ARGS_WT_EC_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_ecBuffer[g_queryAckCount] = argsData[0].f*g_ecBuffer[g_queryAckCount]+argsData[1].f;
        svInt  = data[2+16]<<8;
        svInt |= data[3+16];
        svDec  = data[4+16]<<8;
        svDec |= data[5+16];
        g_phBuffer[g_queryAckCount] = (float)svInt/pow(10, svDec);
        HalFlashRead(HAL_ARGS_WT_PH_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_phBuffer[g_queryAckCount] = argsData[0].f*g_phBuffer[g_queryAckCount]+argsData[1].f;
        svInt  = data[2+24]<<8;
        svInt |= data[3+24];
        svDec  = data[4+24]<<8;
        svDec |= data[5+24];
        g_o2Buffer[g_queryAckCount] = (float)svInt/pow(10, svDec); 
        HalFlashRead(HAL_ARGS_WT_O2_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_o2Buffer[g_queryAckCount] = argsData[0].f*g_o2Buffer[g_queryAckCount]+argsData[1].f;

        svInt  = data[2+28]<<8;
        svInt |= data[3+28];
        svDec  = data[4+28]<<8;
        svDec |= data[5+28];
        g_nh4Buffer[g_queryAckCount] = (float)svInt/pow(10, svDec); 
        HalFlashRead(HAL_ARGS_WT_NH4_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_nh4Buffer[g_queryAckCount] = argsData[0].f*g_nh4Buffer[g_queryAckCount]+argsData[1].f;
            
        svInt  = data[2+32]<<8;
        svInt |= data[3+32];
        svDec  = data[4+32]<<8;
        svDec |= data[5+32];
        g_tbBuffer[g_queryAckCount] = (float)svInt/pow(10, svDec); 
        HalFlashRead(HAL_ARGS_WT_TB_SENSITIVITY_ADDR, (uint8_t *)&argsData[0].b.byte1, 8);
        if(IS_PARAM_LEGAL(argsData[0].f) && IS_PARAM_LEGAL(argsData[1].f))
            g_tbBuffer[g_queryAckCount] = argsData[0].f*g_tbBuffer[g_queryAckCount]+argsData[1].f;
        g_queryAckCount++;
    }
    else
    {
    
    }
}

static void mpsSensorPoll(void)
{
}

static void mpsSensorHalInit(void)
{
}

void MPSSensorModuleInit(void)
{
    mpsSensorHalInit();
    YMSensorType_t type[6] = {YM_SENSOR_TYPE_TP, 
                                YM_SENSOR_TYPE_EC, 
                                YM_SENSOR_TYPE_PH, 
                                YM_SENSOR_TYPE_O2, 
                                YM_SENSOR_TYPE_TB,
                                YM_SENSOR_TYPE_NH4};
    g_handle = SensorCreate(type, 6);

    if(g_handle)
    {
        g_handle->aftQueryWaitTime = 100;
        g_handle->preQueryWaitTime = 5;
        g_handle->poll    = mpsSensorPoll;
        g_handle->prepare = mpsSensorPrepare;
        g_handle->query   = mpsSensorQuery;
        g_handle->realValue = mpsRealValue;
        g_handle->getargs = mpsSensorGetargs;
        g_handle->setargs = mpsSensorSetargs;
        g_handle->power   = mpsSensorPowerctrl;
        SensorRegister(g_handle);
    }

    g_device = SComDeviceRegist(SENSOR_MODBUS_ADDR, LBHA, modbusRecv);

    SensorPowerStatus(g_handle, true);
    //thSensorPowerctrl(true);
}

