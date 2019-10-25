#ifndef YUMAIR_PROTOCOL_H
#define YUMAIR_PROTOCOL_H

#include "YMPrivate.h"

typedef enum
{
    YMSERVER_EVENT_REBOOT_DEVICE = 0, //�����豸
    YMSERVER_EVENT_GET_SENSOR_ARGS,   //��������ֵ
    YMSERVER_EVENT_SET_SENSOR_ARGS,   //д������ֵ
    YMSERVER_EVENT_SET_POST_INTERVAL, //д�ϱ�ʱ����
    YMSERVER_EVENT_SET_SLEEP_MODE,    //��������ģʽ
    YMSERVER_EVENT_CONNECT_STATUS,
    YMSERVER_EVENT_TIMMING_INFO,      //Уʱ��Ϣ
    YMSERVER_EVENT_GET_LOCATION,
    YMSERVER_EVENT_OTA_STATUS,        //ota����״̬
    YMSERVER_EVENT_LINK_ABNORMAL,
}YMServerEvent_t;

typedef void(*YMServerEventHandle_cb)(YMServerEvent_t event, void *opt, YmUint32_t ackid);

const char *YMServerGetPhyUID(void);
int YMServerRequestTiming(void);
void YMServerLocationReport(float latitude, float longitude);
void YMServerReply(YmUint32_t replyID, YMReplyType_t type, YMProcessResult_t result, void * args);
int YMServerPostAllProperties(void);

void YMServerEventHandleRegister(YMServerEventHandle_cb handle);
void YMServerStop(void);
void YMServerStart(void);
void YMServerPoll(void);
void YMServerInitialize(void);

#endif

