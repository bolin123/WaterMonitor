#ifndef YUMAIR_PRIVATE_H
#define YUMAIR_PRIVATE_H

#include "YMCtype.h"
#include "Yumair.h"

//cmd id
//device send
#define YMPROTO_CMD_ONLINE        3021 //现场机上线通知
#define YMPROTO_CMD_ERROR         3022 //现场机上报异常
#define YMPROTO_CMD_TIMING        3020 //请求上位机对时
#define YMPROTO_CMD_DATA_REPORT   2011 //现场机上传数据
#define YMPROTO_CMD_DEV_HEATBEAT  3023
#define YMPROTO_CMD_SET_SLEEPMODE 3025 //设置休眠模式
#define YMPROTO_CMD_GET_SLEEPMODE 3026 //读取休眠模式

//server send
#define YMPROTO_CMD_HEATBEAT_ACK 3024
#define YMPROTO_CMD_SET_TIME     1012 //设置现场机时间
#define YMPROTO_CMD_RESET        3015 //重启现场机
#define YMPROTO_CMD_GET_ARGS     3016 //读取\上传现场机传感器参数
#define YMPROTO_CMD_SET_ARGS     3017 //设置现场机传感器参数
#define YMPROTO_CMD_OTA          3018 //升级现场机固件
#define YMPROTO_CMD_GET_LOCATION 3019 //读取\上传现场机GPS数据
#define YMPROTO_CMD_SET_REPORT_INTERVAL 1062 //设置实时数据上报间隔
#define YMPROTO_CMD_GET_REPORT_INTERVAL 1061 //读取\上传实时数据上报间隔

//应答和结果
#define YMPROTO_CMD_REQUEST_ACK 9011 //请求应答
#define YMPROTO_CMD_OPT_RESULT  9012 //操作执行结果
#define YMPROTO_CMD_NOTICE_ACK  9013 //通知应答


typedef enum
{
    YMPROTO_REQUEST_RETURN_READY = 1, //准备执行请求
    YMPROTO_REQUEST_RETURN_REJECT,    //请求被拒绝
    YMPROTO_REQUEST_RETURN_PWD_ERR,   //密码错误
}YMProtoRequestReturn_t;

typedef enum
{
    YMPROTO_MSG_TYPE_NONE = 0,
    YMPROTO_MSG_TYPE_REPORT, //上传命令,上报实时数据
    YMPROTO_MSG_TYPE_NOTICE,     //通知命令
    YMPROTO_MSG_TYPE_REQUEST,    //请求命令
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

