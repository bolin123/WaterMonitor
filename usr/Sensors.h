#ifndef SENSORS_H
#define SENSORS_H

#include "Sys.h"
#include "Manufacture.h"

#define MIN(X,Y)    ((X)<(Y)?(X):(Y))

/*
* 传感器结构体
* 传感器需要对应实现下面的函数
*/
typedef struct
{   
    SysTime_t preQueryWaitTime;
    SysTime_t aftQueryWaitTime;
    void (*poll)(void);  //主循环
    void (*prepare)(void); //准备开始查询
    void (*query)(void); //查询
    void (*power)(bool on);
    float (*realValue)(YMSensorType_t type);
    void (*setargs)(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid); //设置传感器参数
    void (*getargs)(YMSensorType_t type, uint8_t method, uint32_t ackid); //读取传感器参数
    int8_t (*selfTest)(void**);
    void *private;
}SensorHandle_t;

float SensorCalcAverage(float val[], uint8_t num); //计算均值
void SensorQueryPollDone(SensorHandle_t *handle);
void SensorGotResult(SensorHandle_t *handle);
void SensorPowerStatus(SensorHandle_t *handle, bool poweron);
void SensorReportValue(const char *name, float value, uint8_t flag);
void SensorRecordAvgValue(void);
void SensorGetParamResult(YMSensorType_t type, uint8_t method, float *value, uint8_t valnum, uint32_t ackid); //查询参数结果
void SensorSetParamResult(bool success, uint32_t ackid); //设置参数结果
void SensorGetParam(YMSensorParam_t *args, uint32_t ackid); //查询参数
void SensorSetParam(YMSensorParam_t *args, uint32_t ackid); //设置参数
void SensorFaultReport(uint8_t faultNum, bool set);
bool SensorIsQueryDone(void);
float SensorGetRealValue(YMSensorType_t type);
void SensorSelfTest(void (*fn)(TestType_t a, int8_t b, void* c));

void SensorRegister(SensorHandle_t *handle); //传感器注册
SensorHandle_t *SensorCreate(YMSensorType_t type[], uint8_t typeNum); //创建，type包含的参数类型
void SensorStartQuery(SysTime_t delayTime); //开始
void SensorInitialize(void);
void SensorPoll(void);

#endif

