#ifndef YUMAIR_PRIVATE_H
#define YUMAIR_PRIVATE_H

#include "YMCtype.h"
#include "Yumair.h"

//cmd id
//device send
#define YMPROTO_CMD_ONLINE        3021 //�ֳ�������֪ͨ
#define YMPROTO_CMD_ERROR         3022 //�ֳ����ϱ��쳣
#define YMPROTO_CMD_TIMING        3020 //������λ����ʱ
#define YMPROTO_CMD_DATA_REPORT   2011 //�ֳ����ϴ�����
#define YMPROTO_CMD_DEV_HEATBEAT  3023
#define YMPROTO_CMD_SET_SLEEPMODE 3025 //��������ģʽ
#define YMPROTO_CMD_GET_SLEEPMODE 3026 //��ȡ����ģʽ

//server send
#define YMPROTO_CMD_HEATBEAT_ACK 3024
#define YMPROTO_CMD_SET_TIME     1012 //�����ֳ���ʱ��
#define YMPROTO_CMD_RESET        3015 //�����ֳ���
#define YMPROTO_CMD_GET_ARGS     3016 //��ȡ\�ϴ��ֳ�������������
#define YMPROTO_CMD_SET_ARGS     3017 //�����ֳ�������������
#define YMPROTO_CMD_OTA          3018 //�����ֳ����̼�
#define YMPROTO_CMD_GET_LOCATION 3019 //��ȡ\�ϴ��ֳ���GPS����
#define YMPROTO_CMD_SET_REPORT_INTERVAL 1062 //����ʵʱ�����ϱ����
#define YMPROTO_CMD_GET_REPORT_INTERVAL 1061 //��ȡ\�ϴ�ʵʱ�����ϱ����

//Ӧ��ͽ��
#define YMPROTO_CMD_REQUEST_ACK 9011 //����Ӧ��
#define YMPROTO_CMD_OPT_RESULT  9012 //����ִ�н��
#define YMPROTO_CMD_NOTICE_ACK  9013 //֪ͨӦ��


typedef enum
{
    YMPROTO_REQUEST_RETURN_READY = 1, //׼��ִ������
    YMPROTO_REQUEST_RETURN_REJECT,    //���󱻾ܾ�
    YMPROTO_REQUEST_RETURN_PWD_ERR,   //�������
}YMProtoRequestReturn_t;

typedef enum
{
    YMPROTO_MSG_TYPE_NONE = 0,
    YMPROTO_MSG_TYPE_REPORT, //�ϴ�����,�ϱ�ʵʱ����
    YMPROTO_MSG_TYPE_NOTICE,     //֪ͨ����
    YMPROTO_MSG_TYPE_REQUEST,    //��������
}YMProtoMsgType_t;

typedef enum
{
    YMPROTO_EVENT_REQUEST = 0,
    YMPROTO_EVENT_REQUEST_VALUE,
    YMPROTO_EVENT_RECV_ACK,
    YMPROTO_EVENT_HEARTBEAT,
}YMProtoEvent_t;

typedef struct
{
    YmUint16_t mid;
    char *value;
    char *qn;
}YMProtoServerMsg_t;

typedef int (* YMProtoEventHandle_cb)(int socketid, YMProtoEvent_t event, void *args);
typedef void (* YMProtoDataSend_cb)(int socketid, const char *data, YmUint16_t len);

void YMProtoPostData(int socketid, YmUint16_t msgID, const char *qn, const char *value);

void YMProtoLoginNotice(int socketid, YmUint32_t interval, YmUint8_t err[], YmUint8_t errlen);
void YMProtoErrorReport(int socketid, YmUint8_t errNum[], YmUint8_t len);
void YMProtoReportOptResult(int socketid, const char *qn, YMProcessResult_t result);
void YMProtoHeatbeatSend(int socketid);
YmBool YMProtoSendlistEmpty(void);
void YMProtoClearSendList(int socketid);
char *YMProtoGetCommandParam(const char *msg);

void YMProtoRequestTiming(int socketid);
void YMProtoMessageRecv(int socketid, char *msg, YmUint16_t length);
void YMProtoPropertiesPost(int socketid);

void YMProtoCallbackRegister(YMProtoDataSend_cb sendHandle, YMProtoEventHandle_cb eventHandle);
void YMProtoInitialize(void);
void YMProtoPoll(void);

#endif

