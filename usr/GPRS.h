#ifndef GPRS_H
#define GPRS_H

#include "Sys.h"

typedef uint8_t gprsSocketfd;


/*�¼�*/
typedef enum
{
    GEVENT_POWER_ON = 0,    //GPRS�ϵ�
    GEVENT_POWER_OFF,       //GPRS����
    GEVENT_GPRS_STATUS_CHANGED, //GPRS״̬�ı�
}GPRSEvent_t;

/*GPRS״̬*/
typedef enum
{
    GPRS_STATUS_NONE = 0,    //Ĭ��״̬��δ����ͨ��
    GPRS_STATUS_INITIALIZED, //ͨ������
    GPRS_STATUS_ACTIVE,      //����ȫ����
    GPRS_STATUS_GSM_DONE,    //GSM��ʼ�����
    GPRS_STATUS_GPRS_DONE,   //GPRS��ʼ�����
}GPRSStatus_t;

typedef void (* GPRSTcpDisconnect_cb)(gprsSocketfd id);
typedef void (* GPRSTcpConnect_cb)(gprsSocketfd id, bool success);
typedef void (* GPRSTcpReceive_cb)(gprsSocketfd id, uint8_t *data, uint16_t len);
typedef void (* GPRSTcpSendFail_cb)(gprsSocketfd id, uint8_t *data, uint16_t len);
typedef void (* GPRSEventHandle_cb)(GPRSEvent_t event, void *args);

typedef struct
{
    gprsSocketfd socketId;
    bool connected;
    uint8_t sendFailedNum;
    uint16_t dataLen;
    uint16_t recvCount;
    uint8_t *data;
    GPRSTcpConnect_cb connectCb;
    GPRSTcpDisconnect_cb disconnetCb;
    GPRSTcpReceive_cb recvCb;
    GPRSTcpSendFail_cb sendFailCb;
}GPRSTcpSocket_t;

/*
typedef struct
{
    gprsSocketfd id;
    uint8_t *data;
    uint16_t len;
}GPRSSendData_t;
*/

void GPRSConfigDmaRecv(void);
void GPRSConfigIrqRecv(void);

const char *GPRSGetICCID(void);

/*GPRS�¼�����ע��
* @handle:�¼�������
*/
void GPRSEventHandleRegister(GPRSEventHandle_cb handle);

/*���ý���ģʽ(��������ʱ�������մ�����)
* @entry: 
*    true,����ģʽ(���ٷ���ָ�������)
*    false,�˳�����ģʽ

void GPRSSetRecvMode(bool entry);
*/

/*�ر�TCP����
* @id:����ʱ���ص�id
*/
void GPRSTcpClose(GPRSTcpSocket_t *socket);

/*TCP���ͺ���
* @id:����ʱ���ص�id, @data:���ݣ�@len:���ݳ���
* @return:�������ݳ���
*/
uint16_t GPRSTcpSend(GPRSTcpSocket_t *socket, uint8_t *data, uint16_t len);

/*����TCP����
* @url:��������ַ��@port:�˿ںţ�@callback:���ջص�����
* @return:id
*/
int8_t GPRSTcpConnect(GPRSTcpSocket_t *socket, const char *url, uint16_t port);

void GPRSTcpRelease(GPRSTcpSocket_t *socket);

GPRSTcpSocket_t *GPRSTcpCreate(void);

bool GPRSDatalistEmpty(void);


/*�Ƿ����ӵ�����(��ȡIP��ַ)
* 
*/
bool GPRSConnected(void);

/*��ȡ��ǰGPRS״̬
* @return:
*   GPRS_STATUS_NONE:��״̬��δ����ͨ��
*   GPRS_STATUS_INITIALIZED:ͨ������
*   GPRS_STATUS_ACTIVE:����ȫ����
*   GPRS_STATUS_GSM_DONE:GSM��ʼ�����
*   GPRS_STATUS_GPRS_DONE:GPRS��ʼ�����
*/
GPRSStatus_t GPRSGetStatus(void); 

/*��ȡCSQֵ
* @return: csq value
*/
uint8_t GPRSGetSignalValue(void);

/*gprsͨ�Ŵ����Ƿ�����
* 
*/
bool IsGPRSComOk(void);

/*�ر�GPRS����
* @return:-1:already stop, -2:busy, 0:ok
*/
int8_t GPRSStop(void);

/*��GPRS����
* @return:-1:already start, -2:busy, 0:ok
*/
int8_t GPRSStart(void);

/*��ʼ������*/
void GPRSInitialize(void);

/*ѭ��ִ�к���*/
void GPRSPoll(void);

#endif

