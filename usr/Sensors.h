#ifndef SENSORS_H
#define SENSORS_H

#include "Sys.h"
#include "Manufacture.h"

#define MIN(X,Y)    ((X)<(Y)?(X):(Y))

/*
* �������ṹ��
* ��������Ҫ��Ӧʵ������ĺ���
*/
typedef struct
{   
    SysTime_t preQueryWaitTime;
    SysTime_t aftQueryWaitTime;
    void (*poll)(void);  //��ѭ��
    void (*prepare)(void); //׼����ʼ��ѯ
    void (*query)(void); //��ѯ
    void (*power)(bool on);
    float (*realValue)(YMSensorType_t type);
    void (*setargs)(YMSensorType_t type, uint8_t method, float *args, uint8_t argsNum, uint32_t ackid); //���ô���������
    void (*getargs)(YMSensorType_t type, uint8_t method, uint32_t ackid); //��ȡ����������
    int8_t (*selfTest)(void**);
    void *private;
}SensorHandle_t;

float SensorCalcAverage(float val[], uint8_t num); //�����ֵ
void SensorQueryPollDone(SensorHandle_t *handle);
void SensorGotResult(SensorHandle_t *handle);
void SensorPowerStatus(SensorHandle_t *handle, bool poweron);
void SensorReportValue(const char *name, float value, uint8_t flag);
void SensorRecordAvgValue(void);
void SensorGetParamResult(YMSensorType_t type, uint8_t method, float *value, uint8_t valnum, uint32_t ackid); //��ѯ�������
void SensorSetParamResult(bool success, uint32_t ackid); //���ò������
void SensorGetParam(YMSensorParam_t *args, uint32_t ackid); //��ѯ����
void SensorSetParam(YMSensorParam_t *args, uint32_t ackid); //���ò���
void SensorFaultReport(uint8_t faultNum, bool set);
bool SensorIsQueryDone(void);
float SensorGetRealValue(YMSensorType_t type);
void SensorSelfTest(void (*fn)(TestType_t a, int8_t b, void* c));

void SensorRegister(SensorHandle_t *handle); //������ע��
SensorHandle_t *SensorCreate(YMSensorType_t type[], uint8_t typeNum); //������type�����Ĳ�������
void SensorStartQuery(SysTime_t delayTime); //��ʼ
void SensorInitialize(void);
void SensorPoll(void);

#endif

