#include "YumairProtocol.h"
#include "YMList.h"
#include "YMPropertyManager.h"
#include "YMPrivate.h"

#define YMPROTO_MESSAGE_SEND_RETRIES  3
#define YMPROTO_MESSAGE_SEND_TIMEOUT  20000 //20s

//QN=20040516010101001;ST=22;CN=3022;PW=16944725;MN=YA00100000000043;
#define YMPROTO_NORMAL_MESSAGE_HEAD "QN=%s;ST=%d;CN=%d;PW=%s;MN=%s;Flag=1;"
#define YMPROTO_POST_MESSAGE_HEAD  "ST=%d;CN=%d;PW=%s;MN=%s;"

#define YMPROTO_WTR_QUALITY_ST  21 //水质
#define YMPROTO_AIR_QUALITY_ST  22 //空气质量
#define YMPROTO_SYS_INTERACT_ST 91 //系统交互

#define YMPROTO_CURRENT_ST    YMPROTO_AIR_QUALITY_ST  

//key name
#define YMPROTO_MKEY_QN   "QN="
#define YMPROTO_MKEY_CN   "CN="
#define YMPROTO_MKEY_MN   "MN="
#define YMPROTO_MKEY_PWD  "PW="
#define YMPROTO_MKEY_CP   "CP="
#define YMPROTO_MKEY_FLAG "Flag="

typedef struct YMProtoMsgRetry_st
{
    YmUint8_t retryCount;
    YmUint16_t length;
    int socketid;
    char *contents;
    YmTime_t lastSendTime;
    YMLIST_ENTRY(struct YMProtoMsgRetry_st);
}YMProtoMsgRetry_t;

static YMProtoDataSend_cb g_sendFunc = YmNULL;
static YMProtoEventHandle_cb g_eventHandle = YmNULL;
static YMProtoMsgRetry_t g_msgRetry;

_funcTag static YmUint16_t crc16(YmUint8_t *data, YmUint16_t len)
{
    YmUint16_t i = 0;
    YmUint16_t crc = 0xffff;

    while(len--)
    {
        for(crc ^= *(data++), i = 0; i < 8; i++)
        {
            crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }

    return crc;
}


_funcTag static YMProtoMsgType_t getMsgType(YmUint16_t msgID)
{
    return YMPROTO_MSG_TYPE_REQUEST; //目前服务器下发的消息全部为请求应答
}

_funcTag static char *getDateTimeString(char *str)
{
    YMDateTime_t *time = YM_GET_SYS_DATE_TIME();

    sprintf(str, "%04d%02d%02d%02d%02d%02d%03d", time->year, time->month, time->day, \
        time->hour, time->min, time->sec, time->msec);
    return str;
}

_funcTag static void frameSend(int socketid, YmUint8_t st, YmUint16_t cn, const char *data, YmBool needAck)
{
    YmUint16_t crc;
    YmUint16_t length;
    char crcstr[7] = "";
    char date[17] = "";
    //char frame[800] = "";
    char *frame = YmNULL;
    char *head = YmNULL;
    char *contents = YmNULL;
    YMProtoMsgRetry_t *msgList;

    head = (char *)YmMalloc(100);
    if(head == YmNULL)
    {
        YMLog("no mem!");
        return;
    }
    if(needAck)
    {
        sprintf(head, YMPROTO_NORMAL_MESSAGE_HEAD, getDateTimeString(date), st, cn, \
            YMGetDevPasswd(), YMGetDevID());
    }
    else
    {
        sprintf(head, YMPROTO_POST_MESSAGE_HEAD, st, cn, YMGetDevPasswd(), YMGetDevID());
    }

    length = strlen(head) + strlen(data) + 7; // 7:"CP=&&&&"

    frame = YmMalloc(length + 16);
    if(frame)
    {
    sprintf(frame, "##%04d%sCP=&&%s&&", length, head, data);
    free(head);

    contents = &frame[6];
    crc = crc16((YmUint8_t *)contents, length);
    sprintf(crcstr, "%02x%02x\r\n", (YmUint8_t)(crc >> 8), (YmUint8_t)crc);
    strcat(frame, crcstr);

    //YMLog("%s", frame);
    if(needAck)
    {
        msgList = (YMProtoMsgRetry_t *)YmMalloc(sizeof(YMProtoMsgRetry_t));
        if(msgList)
        {
            msgList->socketid = socketid;
            msgList->length = strlen(frame);
            msgList->retryCount = 0;
            msgList->lastSendTime = 0;
            msgList->contents = YmMalloc(msgList->length + 1); //结束符
            if(msgList->contents)
            {
                memcpy(msgList->contents, frame, msgList->length);
                msgList->contents[msgList->length] = '\0'; // for printf
                YMListAdd(&g_msgRetry, msgList);
            }
            else
            {
                free(msgList);
            }
        }
    }
    else
    {
        g_sendFunc(socketid, frame, strlen(frame));
    }
    free(frame);
    }
}

_funcTag static void msgRetryHandle(void)
{
    YMProtoMsgRetry_t *msg;

    msg = YMListFirst(&g_msgRetry);
    if(msg)
    {
        if(msg->lastSendTime == 0 || YMSysTimeHasPast(msg->lastSendTime, YMPROTO_MESSAGE_SEND_TIMEOUT))
        {
            if(msg->retryCount < YMPROTO_MESSAGE_SEND_RETRIES)
            {

                g_sendFunc(msg->socketid, msg->contents, msg->length);
                msg->lastSendTime = YMSysTime();
                msg->retryCount++;
            }
            else
            {
                YMListDel(msg);
                if(msg->contents)
                {
                    free(msg->contents);
                }
                free(msg);
            }
        }
    }
}

//操作结果
_funcTag void YMProtoReportOptResult(int socketid, const char *qn, YMProcessResult_t result)
{
    char buff[32] = { 0 };
    YMLog("");
    sprintf(buff, "QN=%s;ExeRtn=%d", qn, result);
    frameSend(socketid, YMPROTO_SYS_INTERACT_ST, YMPROTO_CMD_OPT_RESULT, buff, YmFalse);
}

//请求应答
_funcTag static void requestAckSend(int socketid, const char *qn, YMProtoRequestReturn_t result)
{
    char buff[32] = { 0 };
    YMLog("");
    sprintf(buff, "QN=%s;QnRtn=%d", qn, result);
    frameSend(socketid, YMPROTO_SYS_INTERACT_ST, YMPROTO_CMD_REQUEST_ACK, buff, YmFalse);
}

//通知应答
_funcTag static void noticeAckSend(int socketid, const char *qn)
{
    char buff[32] = { 0 };
    YMLog("");
    sprintf(buff, "QN=%s", qn);
    frameSend(socketid, YMPROTO_SYS_INTERACT_ST, YMPROTO_CMD_NOTICE_ACK, buff, YmFalse);
}

_funcTag static char *getValueFromMsg(const char *name, const char *msg)
{
    char *pos, *end = YmNULL;
    char *value;

    pos = strstr(msg, name);
    if(pos)
    {
        pos = strchr(pos, '=');
        if(pos)
        {
            pos++;

            if(strchr(pos, ';'))
            {
                end = strchr(pos, ';');
            }
            else if(strchr(pos, '&'))
            {
                end = strchr(pos, '&');
            }
            else if(strchr(pos, ','))
            {
                end = strchr(pos, ',');
            }
            else
            {
                end = YmNULL;
            }

            if(end)
            {
                value = YmMalloc(end - pos + 1);
                if(value)
                {
                    memcpy(value, pos, end - pos);
                    value[end - pos] = '\0';
                    return value;
                }
                else
                {
                    return YmNULL;
                }
            }
        }
    }
    return YmNULL;
}

_funcTag static char *getCPTextFromMsg(const char *msg)
{
    char *pos, *end;
    char *text;

    pos = strstr(msg, YMPROTO_MKEY_CP);
    if(pos)
    {
        pos = strstr(pos, "&&");
        if(pos)
        {
            pos += 2;
            end = strstr(pos, "&&");
            if(end)
            {
                text = YmMalloc(end - pos + 1);
                memcpy(text, pos, end - pos);
                text[end - pos] = '\0';
                return text;
            }
        }
    }
    return YmNULL;
}

_funcTag static void findAndDelSendCache(int socketid, const char *qn)
{
    YMProtoMsgRetry_t *msg;
    YmUint32_t msgID;
    char *idStr;

    YMListForeach(&g_msgRetry, msg)
    {
        if(strstr(msg->contents, qn) != YmNULL && msg->socketid == socketid)
        {
            YMLog("del, qn=%s", qn);
            idStr = getValueFromMsg(YMPROTO_MKEY_CN, msg->contents);
            if(idStr)
            {
                msgID = strtol(idStr, YmNULL, 10);
                g_eventHandle(socketid, YMPROTO_EVENT_RECV_ACK, (void *)msgID);
                free(idStr);
            }

            YMListDel(msg);
            if(msg->contents)
            {
                free(msg->contents);
            }
            free(msg);
            return;
        }
    }

    YMLog("[sockid:%d] qn = %s not found!", socketid, qn);
}

_funcTag static void msgParse(int socketid, const char *msg, YmUint16_t len)
{
    char *qnVal;
    char *cnVal;
    char *cpVal;
    YmUint16_t msgID;
    YmUint8_t requestResult;
    //YMProtoMsgType_t msgType;
    YMProtoServerMsg_t setMsg;

    //char *flag;

    qnVal = getValueFromMsg(YMPROTO_MKEY_QN, msg);

    cnVal = getValueFromMsg(YMPROTO_MKEY_CN, msg);
    if(cnVal == YmNULL)
    {
        goto DataFree;
    }
    //flag  = getValueFromMsg(YMPROTO_MKEY_FLAG, msg);
    cpVal = getCPTextFromMsg(msg);

    msgID = (YmUint16_t)(YmUint32_t)strtol(cnVal, YmNULL, 10);

    if(msgID == YMPROTO_CMD_NOTICE_ACK || msgID == YMPROTO_CMD_REQUEST_ACK)
    {
        if(qnVal)
        {
            YMLog("ack, qn=%s", qnVal);
            findAndDelSendCache(socketid, qnVal);
        }
        goto DataFree;
    }

    if(msgID == YMPROTO_CMD_HEATBEAT_ACK)
    {
        g_eventHandle(socketid, YMPROTO_EVENT_HEARTBEAT, YmNULL);
        goto DataFree;
    }

    switch(getMsgType(msgID))
    {
    case YMPROTO_MSG_TYPE_NOTICE:     //通知命令
        noticeAckSend(socketid, qnVal);
        break;
    case YMPROTO_MSG_TYPE_REQUEST:    //请求命令
        requestResult = g_eventHandle(socketid, YMPROTO_EVENT_REQUEST, (void *)(YmUint32_t)msgID);
        requestAckSend(socketid, qnVal, (YMProtoRequestReturn_t)requestResult);
        break;
    default:
        break;
    }

    setMsg.mid = msgID;
    setMsg.value = cpVal;
    setMsg.qn = qnVal;
    g_eventHandle(socketid, YMPROTO_EVENT_REQUEST_VALUE, &setMsg);

DataFree:
    if(qnVal)
    {
        free(qnVal);
        qnVal = YmNULL;
    }
    if(cnVal)
    {
        free(cnVal);
        cnVal = YmNULL;
    }
    if(cpVal)
    {
        free(cpVal);
        cpVal = YmNULL;
    }
}

_funcTag char *YMProtoGetCommandParam(const char *msg)
{
    YmUint16_t msgID;
    char *cnVal = getValueFromMsg(YMPROTO_MKEY_CN, msg);

    if(cnVal)
    {
        msgID = (YmUint16_t)(YmUint32_t)strtol(cnVal, YmNULL, 10);
        free(cnVal);
        if(YMPROTO_CMD_DATA_REPORT == msgID)
        {
            return getCPTextFromMsg(msg);
        }
        else
        {
            return YmNULL;
        }
    }
    return YmNULL;

}

_funcTag void YMProtoMessageRecv(int socketid, char *msg, YmUint16_t length)
{
    YmUint16_t i, j , n;
    YmUint16_t startPos = 0;
    char *msgStart = YmNULL;
    char *contents = YmNULL;
    char *crc = YmNULL;
    YmUint16_t crcValue;
    YmUint16_t contentLen;

    YMPrint("Msg recv [%d]: %s\n", socketid, msg);

    for(i = 0, n = 0; i < length; i++) //有可能同时收到多条
    {
        if(msg[i] == '\r')
        {
            msg[i] = '\0';
            n++;
        }
    }

    for(j = 0; j < n; j++)
    {
        for(i = startPos; i < length; i++)
        {
            if(msg[i] == '#' && msg[i + 1] == '#') //取消息头
            {
                msgStart = &msg[i];
                break;
            }
        }

        if(msgStart)
        {
            contentLen = (YmUint16_t)(YmUint32_t)strtol(&msgStart[2], YmNULL, 10);
            contents = &msgStart[2 + 4];
            crc = &msgStart[2 + 4 + contentLen]; //包头2B + 数据段长度4B + 数 据 段
            crcValue = (YmUint16_t)(YmUint32_t)strtol(crc, YmNULL, 16);
            if(crcValue == crc16((YmUint8_t *)contents, contentLen))
            {
                msgParse(socketid, contents, contentLen);
            }
            else
            {
                YMLog("crc error!");
            }
            startPos = 2 + 4 + contentLen + 4 + 2; //包头2B + 数据段长度4B + 数 据 段 + CRC 校验4B + 包尾2B
        }
    }

}

_funcTag void YMProtoPostData(int socketid, YmUint16_t msgID, const char *qn, const char *value)
{
    char *buff = YmNULL;

    YMLog("");
    if(qn != YmNULL)
    {
        buff = YmMalloc(strlen(qn) + strlen(value) + 16);
        if(buff)
        {
            sprintf(buff, "QN=%s;%s", qn, value);
            frameSend(socketid, YMPROTO_CURRENT_ST, msgID, buff, YmFalse);
            free(buff);
        }
    }
    else
    {
        frameSend(socketid, YMPROTO_CURRENT_ST, msgID, value, YmFalse);
    }
}

_funcTag void YMProtoHeatbeatSend(int socketid)
{
    char *hb = "##0008CN=3023;2f51\r\n";
    YMLog("");
    g_sendFunc(socketid, hb, strlen(hb));
}

_funcTag void YMProtoClearSendList(int socketid)
{
    YMProtoMsgRetry_t *node;

    YMListForeach(&g_msgRetry, node)
    {
        if(node->socketid == socketid)
        {
            YMListDel(node);
            if(node->contents)
            {
                free(node->contents);
            }
            free(node);
        }
    }
}

_funcTag YmBool YMProtoSendlistEmpty(void)
{
    return YMListFirst(&g_msgRetry) == YmNULL;
}

//请求授时
_funcTag void YMProtoRequestTiming(int socketid)
{
    YMLog("");
    frameSend(socketid, YMPROTO_CURRENT_ST, YMPROTO_CMD_TIMING, "", YmTrue);
}

//上报异常
_funcTag void YMProtoErrorReport(int socketid, YmUint8_t errNum[], YmUint8_t num)
{
    YmUint8_t i, len = 0;
    char buff[128] = "ErrorCode=[";
    char *err = &buff[strlen("ErrorCode=[")];

    if(num != 0)
    {
        for(i = 0; i < num - 1; i++)
        {
            len += sprintf(err + len, "%d,", errNum[i]);
        }
        sprintf(err + len, "%d", errNum[num - 1]);
    }
    strcat(err, "];");
    YMLog("%s", buff);
    frameSend(socketid, YMPROTO_CURRENT_ST, YMPROTO_CMD_ERROR, buff, YmTrue);
}

_funcTag static const char *firmVersion2String(void)
{
    static char verstr[12] = "";
    YmUint8_t *vernum = (YmUint8_t *)YMGetFirmVersion();

    sprintf(verstr, "%d.%d.%d.%d", vernum[0], vernum[1], vernum[2], vernum[3]);
    return verstr;
}

//上线通知
_funcTag void YMProtoLoginNotice(int socketid, YmUint32_t interval, YmUint8_t err[], YmUint8_t errlen)
{
    char buff[256];
    char *errcode;
    YmUint8_t i, len = 0;

    YMLog("");
    //ICCID=898600810906f8048812;RtdInterval=60;FMW-Type=yumair01;FMW-Version=1.0.0.1

    if(errlen)
    {
        sprintf(buff, "ICCID=%s;RtdInterval=%d;FMW-Type=%s;FMW-Version=%s;ErrorCode=[", \
            YMServerGetPhyUID(), \
            interval, \
            YMGetDeviceModel(), \
            firmVersion2String());
        errcode = &buff[strlen(buff)];
        for(i = 0; i < errlen - 1; i++)
        {
            len += sprintf(errcode + len, "%d,", err[i]);
        }
        sprintf(errcode + len, "%d];", err[errlen - 1]);
    }
    else
    {
        sprintf(buff, "ICCID=%s;RtdInterval=%d;FMW-Type=%s;FMW-Version=%s;ErrorCode=[]", \
            YMServerGetPhyUID(), \
            interval, \
            YMGetDeviceModel(), \
            firmVersion2String());
    }

    frameSend(socketid, YMPROTO_CURRENT_ST, YMPROTO_CMD_ONLINE, buff, YmTrue);
}

//上传数据
_funcTag void YMProtoPropertiesPost(int socketid)
{
    char *buff = YmNULL;

    buff = (char *)YMPMProperties2Text();
    frameSend(socketid, YMPROTO_CURRENT_ST, YMPROTO_CMD_DATA_REPORT, buff, YmFalse);
    free(buff);

}


_funcTag void YMProtoCallbackRegister(YMProtoDataSend_cb sendHandle, YMProtoEventHandle_cb eventHandle)
{
    g_sendFunc = sendHandle;
    g_eventHandle = eventHandle;
}

_funcTag void YMProtoPoll(void)
{
    msgRetryHandle();
}

_funcTag void YMProtoInitialize(void)
{
    YMListInit(&g_msgRetry);
}

