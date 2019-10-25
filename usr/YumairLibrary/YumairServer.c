#include "YumairServer.h"
#include "YumairProtocol.h"
#include "YumairOTA.h"
#include "YMList.h"
#include "YMSocket.h"
#include "YMPropertyManager.h"

#define YM_SERVER_QN_NUM_LEN  18
#define YM_SERVER_REPLY_TIMEOUT  120000 //2min 应答超时时间

#define YM_SERVER_LOGIN_TIMEOUT_MAX 60000 //1min

#if YM_SLAVE_SERVER_ENABLE
#define YM_SERVER_LINK_NUM 2
#else
#define YM_SERVER_LINK_NUM 1
#endif // YM_SLAVE_SERVER_ENABLE

typedef struct YMServerReplyQn_st
{
    int socketid;
    char *qnString;
    YmTime_t timeout;
    YmUint32_t replyID;
    YMLIST_ENTRY(struct YMServerReplyQn_st);
}YMServerReply_t;

typedef struct
{
    YmTime_t lastReportTime;
    YmUint64_t lastNum;
    YmUint64_t currentNum;
}YPFault_t;

typedef struct
{
    YmUint8_t serverID;
    YmUint8_t retryCount;
    char *serverUrl;
    YmUint16_t serverPort;
    YMServerLink_t serverStatus;
    YmTime_t lastActiveTime;
    YMSocket_t *nm;
#if YM_RESUME_TRANSFER_ENABLE
    YmBool startRetransfer;
    YmTime_t lastRetransferTime;
#endif
}YMServerNetwork_t;

static YMServerNetwork_t g_serverNet[YM_SERVER_LINK_NUM];
static YmBool g_serverStart = YmFalse;
static YMServerEventHandle_cb g_eventHandle = YmNULL;
static YMServerReply_t g_serverReply;

_funcTag static void serverEventEmit(YMServerEvent_t event, void *opt, YmUint32_t ackid)
{
    if(g_eventHandle)
    {
        g_eventHandle(event, opt, ackid);
    }
}

_funcTag static void serverStatusChange(YMServerNetwork_t *net, YMServerLink_t status)
{
    YMServerStatus_t server;

    if(net && net->serverStatus != status)
    {
        YMLog("sever status %d -> %d", net->serverStatus, status);

        net->serverStatus = status;
        net->retryCount = 0;
        net->lastActiveTime = 0;

        server.sid = (YMServerId_t)net->serverID;
        server.status = status;
        serverEventEmit(YMSERVER_EVENT_CONNECT_STATUS, (void *)&server, 0);
    }
}

_funcTag static void faultCheck(void)
{
    YmUint8_t i;
    YmUint8_t fbuff[64];
    YmUint8_t faultCount;
    static YmTime_t lastCheckTime;

    if(YMSysTimeHasPast(lastCheckTime, 10000))
    {
        faultCount = YMGetFaultNum(fbuff, YmFalse);
        if(faultCount)
        {
            for(i = 0; i < YM_SERVER_LINK_NUM; i++)
            {
                if(g_serverNet[i].serverStatus == YM_SERVER_LINK_LOGIN)
                {
                    YMProtoErrorReport((int)g_serverNet[i].nm, fbuff, faultCount);
                }
            }
        }
        lastCheckTime = YMSysTime();
    }
}

#if YM_RESUME_TRANSFER_ENABLE
#define YM_RETRANS_LOAD_BUFF_SIZE 1024
_funcTag static void breakpointResume(void)
{
    YmUint8_t i;
    char *buff = YmNULL;
    YMServerNetwork_t *netInfo;

    for(i = 0; i < YM_SERVER_LINK_NUM; i++)
    {
        netInfo = &g_serverNet[i];
        if(netInfo->startRetransfer
            && netInfo->serverStatus == YM_SERVER_LINK_LOGIN
            && YMSysTimeHasPast(netInfo->lastRetransferTime, 20000))
        {
            buff = YmMalloc(YM_RETRANS_LOAD_BUFF_SIZE);
            if(buff)
            {
                if(YM_RETRANS_LOAD_NEXT_DATA(netInfo->serverID, buff, YM_RETRANS_LOAD_BUFF_SIZE) > 0)
                {
                    YMLog("lost msg:%s", buff);
                    YMProtoPostData((int)netInfo->nm, YMPROTO_CMD_DATA_REPORT, YmNULL, buff);
                    YM_RETRANS_DEL_CURRENT_DATA(i);
                }
                else
                {
                    netInfo->startRetransfer = YmFalse;
                }
                free(buff);
            }

            netInfo->lastRetransferTime = YMSysTime();
        }
    }
}
#endif

_funcTag int YMServerPostAllProperties(void)
{
    YmUint8_t i;

    YMLog("");

    for(i = 0; i < YM_SERVER_LINK_NUM; i++)
    {
        if(g_serverNet[i].serverStatus == YM_SERVER_LINK_LOGIN)
        {
            YMProtoPropertiesPost((int)g_serverNet[i].nm);
#if YM_RESUME_TRANSFER_ENABLE
            g_serverNet[i].lastRetransferTime = YMSysTime();
#endif
        }
#if YM_RESUME_TRANSFER_ENABLE
        else
        {
            char *text = (char *)YMPMProperties2Text();
            YMLog("RETRANS_SAVE:%s", text);
            YM_RETRANS_SAVE_LOST_DATA(g_serverNet[i].serverID, text);
            free(text);
        }
#endif
    }
    return 0;
}

_funcTag int YMServerRequestTiming(void)
{
    YmUint8_t i;

    for(i = 0; i < YM_SERVER_LINK_NUM; i++)
    {
        if(g_serverNet[i].serverStatus == YM_SERVER_LINK_LOGIN)
        {
            YMProtoRequestTiming((int)g_serverNet[i].nm);
            return 0; //有一个授时即可
        }
    }

    return -1;
}

_funcTag void YMServerEventHandleRegister(YMServerEventHandle_cb handle)
{
    g_eventHandle = handle;
}

_funcTag static void serverDataSend(int socketid, const char *data, YmUint16_t len)
{
    YMSocket_t *nm = (YMSocket_t *)socketid;

    if(nm)
    {
        YMSocketTcpSend(nm, (YmUint8_t *)data, len);
    }
}

_funcTag void YMServerLocationReport(float latitude, float longitude)
{
    YmUint8_t i;
    char buff[80];
    YMServerNetwork_t *netInfo;
    YMDateTime_t *date = YM_GET_SYS_DATE_TIME();

    YMLog("latitude = %f, longitude = %f", latitude, longitude);

    //DataTime=20040516020111;LON-Rtd=116.604630;LAT-Rtd=35.442291
    sprintf(buff, "DataTime=%04d%02d%02d%02d%02d%02d;LON-Rtd=%f;LAT-Rtd=%f", \
        date->year, \
        date->month, \
        date->day, \
        date->hour, \
        date->min, \
        date->sec, \
        longitude, \
        latitude);

    for(i = 0; i < YM_SERVER_LINK_NUM; i++)
    {
        netInfo = &g_serverNet[i];
        if(netInfo->serverStatus == YM_SERVER_LINK_LOGIN)
        {
            YMProtoPostData((int)netInfo->nm, YMPROTO_CMD_DATA_REPORT, YmNULL, buff);
        }
    }
}

//返回上位机请求的结果
_funcTag static YMProtoRequestReturn_t deviceStatusQuery(YmUint16_t msgID)
{
    //todo:
    return YMPROTO_REQUEST_RETURN_READY;
}

_funcTag static YMDateTime_t *dateStringConvert(const char *timestr)
{
    static YMDateTime_t dataTime;
    char *keyName = strstr(timestr, "SystemTime=");
    char *time;
    char data[5] = { 0 };

    if(keyName)
    {
        time = strchr(keyName, '=');
        if(time)
        {
            time += 1;
            memcpy(data, time, 4); //year
            dataTime.year = atoi(data);

            time += 4;
            memcpy(data, time, 2); //month
            data[2] = '\0';
            dataTime.month = atoi(data);

            time += 2;
            memcpy(data, time, 2); //day
            data[2] = '\0';
            dataTime.day = atoi(data);

            time += 2;
            memcpy(data, time, 2); //hour
            data[2] = '\0';
            dataTime.hour = atoi(data);

            time += 2;
            memcpy(data, time, 2); //min
            data[2] = '\0';
            dataTime.min = atoi(data);

            time += 2;
            memcpy(data, time, 2); //sec
            data[2] = '\0';
            dataTime.sec = atoi(data);

            return &dataTime;
        }
    }
    return YmNULL;
}

_funcTag static void delServerReply(YMServerReply_t *reply)
{
    if(reply)
    {
        YMListDel(reply);
        if(reply->qnString)
        {
            free(reply->qnString);
        }
        free(reply);
    }
}

_funcTag static void serverReplyPoll(void)
{
    YMServerReply_t *reply;

    YMListForeach(&g_serverReply, reply)
    {
        if(YMSysTimeHasPast(reply->timeout, YM_SERVER_REPLY_TIMEOUT))
        {
            YMLog("opt qn=%s timeout!", reply->qnString);
            YMProtoReportOptResult(reply->socketid, reply->qnString, YM_PROCESS_RESULT_FAILED);
            delServerReply(reply);
        }
    }
}

_funcTag static YmUint32_t serverReplyQnSave(int socketid, const char *qnNum)
{
    YMServerReply_t *qn;

    qn = (YMServerReply_t *)YmMalloc(sizeof(YMServerReply_t));
    if(qn)
    {
        qn->qnString = (char *)YmMalloc(YM_SERVER_QN_NUM_LEN);
        if(qn->qnString)
        {
            strncpy(qn->qnString, qnNum, YM_SERVER_QN_NUM_LEN);
        }
        qn->socketid = socketid;
        qn->replyID = YMSysTime();
        qn->timeout = YMSysTime();
        YMListAdd(&g_serverReply, qn);
        return qn->replyID;
    }
    return 0;
}

_funcTag YMServerReply_t *serverReplyQnLoad(YmUint32_t qnID)
{
    YMServerReply_t *qn = YmNULL;

    YMListForeach(&g_serverReply, qn)
    {
        if(qn->replyID == qnID)
        {
            return qn;
        }
    }

    return YmNULL;
}

_funcTag static int sensorParamSend(int socket, YMSensorParam_t *param, const char *qnstr)
{
    YmUint8_t i;
    YmUint16_t len = 0;
    char *buff = YmNULL;

    YMLog("");

    if(param->value != YmNULL)
    {
        buff = YmMalloc(param->valnum * 15 + 50); //15-float value length, 50 others

        if(buff)
        {
            //Calib-Method=2,Calib-Target=PM2.5,Calib-Param=[1,2]
            len += sprintf(buff, "Calib-Method=%d,Calib-Target=%s,Calib-Param=[", param->method, YMSensorName[param->target]);
            for(i = 0; i < param->valnum - 1; i++)
            {
                len += sprintf(buff + len, "%f,", param->value[i]);
            }
            len += sprintf(buff + len, "%f]", param->value[param->valnum - 1]);
            YMProtoPostData(socket, YMPROTO_CMD_GET_ARGS, qnstr, buff);
            free(buff);
            return 0;
        }
    }
    return -1;
}

_funcTag void YMServerReply(YmUint32_t replyID, YMReplyType_t type, YMProcessResult_t result, void * args)
{
    char buff[64] = "";
    YMServerReply_t *reply = serverReplyQnLoad(replyID);
    YMLocationInfo_t *location = (YMLocationInfo_t *)args;
    YMSensorParam_t *param = (YMSensorParam_t *)args;

    if(reply)
    {
        switch(type)
        {
        case YM_REPLY_TYPE_LOCATION:
            if(location)
            {
                sprintf(buff, "LON-Rtd=%f;LAT-Rtd=%f", location->longitude, location->latitude);
                YMProtoPostData(reply->socketid, YMPROTO_CMD_GET_LOCATION, reply->qnString, buff);
            }
            break;
        case YM_REPLY_TYPE_SENSOR_PARAM:
            if(param)
            {
                sensorParamSend(reply->socketid, param, reply->qnString);
            }
            break;
        case YM_REPLY_TYPE_RESULT:
            break;
        default:
            break;
        }
        YMProtoReportOptResult(reply->socketid, reply->qnString, result);
        delServerReply(reply);
    }
}

_funcTag static YmInt8_t parseSensorParam(const char *text, YMSensorParam_t *param)
{
    YmUint8_t i;
    char *ptr = YmNULL;
    char *paramBegin, *paramEnd;
    YmUint8_t count = 0;

    YMLog("%s", text);
    if(param && text)
    {
        ptr = strstr(text, "Calib-Method=");
        if(ptr)
        {
            ptr = &ptr[strlen("Calib-Method=")];
            param->method = (YmUint8_t)atoi(ptr);
        }
        else
        {
            YMLog("not found method!");
            return -1;
        }

        ptr = strstr(text, "Calib-Target=");
        if(ptr)
        {
            ptr = &ptr[strlen("Calib-Target=")];

            for(i = 0; i < YM_SENSOR_TYPE_COUNT; i++)
            {
                if(strstr(ptr, YMSensorName[YM_SENSOR_TYPE_COUNT-1-i]))
                {
                    param->target = (YMSensorType_t)(YM_SENSOR_TYPE_COUNT-1-i);
                    break;
                }
            }
        }
        else
        {
            YMLog("not found target!");
            return -2;
        }

        ptr = strstr(text, "Calib-Param=");
        if(ptr)
        {
            ptr = &ptr[strlen("Calib-Param=")];
            paramBegin = strchr(ptr, '[');
            paramEnd = strchr(ptr, ']');
            count = 0;
            if(paramBegin && paramEnd && (paramEnd - paramBegin > 1))
            {
                ptr = paramBegin;
                while(ptr != YmNULL && ptr < paramEnd)
                {
                    count++;
                    ptr++;
                    ptr = strchr(ptr, ',');
                }
                param->valnum = count;
                param->value = YmMalloc(sizeof(float) * count);

                ptr = paramBegin + 1;
                for(i = 0; i < count; i++)
                {
                    param->value[i] = atof(ptr);
                    ptr = strchr(ptr, ',') + 1;
                }
            }
        }
        else
        {
            param->value = YmNULL;
            param->valnum = 0;
        }
    }
    return 0;
}

_funcTag static void otaEvenetHandle(YMOTAEvent_t event, void *args, YmUint32_t replyID)
{
    YMOTAData_t otaData;

    switch(event)
    {
    case YMOTA_EVENT_START:
        otaData.status = YM_OTA_STATUS_START;
        otaData.info = (YMOTAInfo_t *)args;
        break;
    case YMOTA_EVENT_PROGRESS:
        otaData.status = YM_OTA_STATUS_PROGRESS;
        otaData.progress = (YmUint8_t)(YmUint32_t)args;
        break;
    case YMOTA_EVENT_SUCCESS:
        otaData.status = YM_OTA_STATUS_RESULT_SUCCESS;
        otaData.info = (YMOTAInfo_t *)args;
        YMServerReply(replyID, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_SUCCESS, YmNULL);
        break;
    case YMOTA_EVENT_FAILED:
        otaData.status = YM_OTA_STATUS_RESULT_FAILED;
        otaData.info = (YMOTAInfo_t *)args;
        YMServerReply(replyID, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, YmNULL);
        break;
    default:
        break;
    }
    serverEventEmit(YMSERVER_EVENT_OTA_STATUS, (void *)&otaData, 0);
}

_funcTag static void otaCommentParse(char *msg, YmUint32_t replyID)
{
    YmUint8_t i;
    YmUint8_t version[4];
    char verstr[3];
    char *point;
    YmUint32_t size;
    char *url;
    YmUint16_t urlLen = 0;
    char *ptr, *urlstart;
    char md5str[3] = "";
    YmUint8_t md5[16];

    YMLog("%s", msg);
    /*
    * FMW-Type=yumair01,FMW-Version=1.0.0.1,FMW-Size=12345,
    * FMW-MD5=E2FC714C4727EE9395F324CD2E7F331F,URL=xxx.xxx
    */
    ptr = strstr(msg, "FMW-Size=");
    if(ptr)
    {
        size = strtol(&ptr[strlen("FMW-Size=")], YmNULL, 10);
    }

    ptr = strstr(msg, "FMW-MD5=");
    if(ptr)
    {
        ptr = &ptr[strlen("FMW-MD5=")];
        for(i = 0; i < 16; i++)
        {
            memcpy(md5str, ptr, 2);
            md5[i] = (YmUint8_t)strtol(md5str, YmNULL, 16);
            ptr += 2;
        }
    }

    ptr = strstr(msg, "FMW-Version=");
    if(ptr)
    {
        ptr = &ptr[strlen("FMW-Version=")];
        for(i = 0; i < 3; i++)
        {
            point = strchr(ptr, '.');
            if(point)
            {
                memset(verstr, 0, sizeof(verstr));
                memcpy(verstr, ptr, point - ptr);
                version[i] = atoi(verstr);
                ptr = point + 1;
            }
        }
        version[3] = (YmUint8_t)strtol(ptr, YmNULL, 10);
    }

    ptr = strstr(msg, "URL=");
    if(ptr)
    {
        urlstart = &ptr[strlen("URL=")];

        if(strchr(urlstart, ';'))
        {
            urlLen = strchr(urlstart, ';') - urlstart;
        }
        else
        {
            urlLen = strlen(urlstart);
        }

        if(urlLen)
        {

            url = (char *)YmMalloc(urlLen + 1);
            if(url)
            {
                memset(url, 0, urlLen + 1);
                memcpy(url, urlstart, urlLen);
                if(YMOTAStart(url, size, md5, version, replyID) < 0)
                {
                    YMServerReply(replyID, YM_REPLY_TYPE_RESULT, YM_PROCESS_RESULT_FAILED, YmNULL);
                }

                free(url);
            }
        }
    }
}

_funcTag static void serverRequestHandle(int socketid, YMProtoServerMsg_t *msg)
{
    int interval;
    char *ptr = YmNULL;
    char data[128] = { 0 };
    YmUint8_t sleepMode;
    YMSensorParam_t sersorParam;
    YMDateTime_t *date;

    YMLog("msgID = %d", msg->mid);

    switch(msg->mid)
    {
    case YMPROTO_CMD_SET_TIME: //同步时间
        date = dateStringConvert(msg->value);
        YMSetDateTime(date);
        serverEventEmit(YMSERVER_EVENT_TIMMING_INFO, (void *)date, 0);
        YMProtoReportOptResult(socketid, msg->qn, YM_PROCESS_RESULT_SUCCESS);
        break;
    case YMPROTO_CMD_GET_REPORT_INTERVAL: //读取上报间隔
        sprintf(data, "RtdInterval=%d", YMGetReportInterval());
        YMProtoPostData(socketid, msg->mid, msg->qn, data);
        YMProtoReportOptResult(socketid, msg->qn, YM_PROCESS_RESULT_SUCCESS);
        break;
    case YMPROTO_CMD_SET_REPORT_INTERVAL: //设置上报间隔
        ptr = strstr(msg->value, "RtdInterval=");
        if(ptr)
        {
            ptr = &ptr[strlen("RtdInterval=")];
            interval = strtol(ptr, YmNULL, 10);
            serverEventEmit(YMSERVER_EVENT_SET_POST_INTERVAL, \
                (void *)interval, \
                serverReplyQnSave(socketid, msg->qn));
        }
        break;
    case YMPROTO_CMD_RESET: //复位
        serverEventEmit(YMSERVER_EVENT_REBOOT_DEVICE, \
            YmNULL, \
            serverReplyQnSave(socketid, msg->qn));
        break;
    case YMPROTO_CMD_GET_ARGS: //读取参数
        sersorParam.value = YmNULL;
        if(parseSensorParam(msg->value, &sersorParam) < 0)
        {
            YMProtoReportOptResult(socketid, msg->qn, YM_PROCESS_RESULT_FAILED);
        }
        else
        {
            serverEventEmit(YMSERVER_EVENT_GET_SENSOR_ARGS, \
                &sersorParam, \
                serverReplyQnSave(socketid, msg->qn));
        }

        if(sersorParam.value != YmNULL)
        {
            free(sersorParam.value);
            sersorParam.value = YmNULL;
        }
        break;
    case YMPROTO_CMD_SET_ARGS: //设置参数
        sersorParam.value = YmNULL;
        if(parseSensorParam(msg->value, &sersorParam) < 0)
        {
            YMProtoReportOptResult(socketid, msg->qn, YM_PROCESS_RESULT_FAILED);
        }
        else
        {
            serverEventEmit(YMSERVER_EVENT_SET_SENSOR_ARGS, \
                &sersorParam, \
                serverReplyQnSave(socketid, msg->qn));
        }

        if(sersorParam.value)
        {
            free(sersorParam.value);
            sersorParam.value = YmNULL;
        }
        break;
    case YMPROTO_CMD_OTA:
#if YM_OTA_ENABLE
        otaCommentParse(msg->value, serverReplyQnSave(socketid, msg->qn));
#endif
        break;
    case YMPROTO_CMD_GET_LOCATION: //定位
        serverEventEmit(YMSERVER_EVENT_GET_LOCATION, YmNULL, serverReplyQnSave(socketid, msg->qn));
        break;
    case YMPROTO_CMD_SET_SLEEPMODE:
        ptr = strstr(msg->value, "SleepMode=");
        if(ptr)
        {
            ptr = &ptr[strlen("SleepMode=")];
            sleepMode = strtol(ptr, YmNULL, 10);
            serverEventEmit(YMSERVER_EVENT_SET_SLEEP_MODE, \
                (void *)sleepMode, \
                serverReplyQnSave(socketid, msg->qn));
        }
        break;
    case YMPROTO_CMD_GET_SLEEPMODE:
        sprintf(data, "SleepMode=%d", YMGetSleepMode());
        YMProtoPostData(socketid, msg->mid, msg->qn, data);
        YMProtoReportOptResult(socketid, msg->qn, YM_PROCESS_RESULT_SUCCESS);
        break;
    default:
        break;
    }
}

_funcTag static int protocolEventHandle(int socketid, YMProtoEvent_t event, void *args)
{
    int ret = 0;
    YmUint32_t value;
    YMSocket_t *nm = (YMSocket_t *)socketid;

    YMLog("event = %d", event);
    switch(event)
    {
    case YMPROTO_EVENT_REQUEST:  //请求需要返回请求结果
        ret = deviceStatusQuery((YmUint16_t)(YmUint32_t)args);
        break;
    case YMPROTO_EVENT_REQUEST_VALUE:
        serverRequestHandle(socketid, (YMProtoServerMsg_t *)args);
        break;
    case YMPROTO_EVENT_RECV_ACK:
        value = (YmUint32_t)args;
        if(value == YMPROTO_CMD_ONLINE)
        {
#if YM_RESUME_TRANSFER_ENABLE
            ((YMServerNetwork_t *)nm->private)->startRetransfer = YmTrue;
            ((YMServerNetwork_t *)nm->private)->lastRetransferTime = YMSysTime();
#endif
            serverStatusChange((YMServerNetwork_t *)nm->private, YM_SERVER_LINK_LOGIN);
        }
        break;
    case YMPROTO_EVENT_HEARTBEAT:
        YMPrint("hearbeat \n");
        break;
    default:
        break;
    }
    return ret;
}

_funcTag static void loginServer(YMServerNetwork_t *net)
{
    YmUint8_t errcount = 0;
    YmUint8_t errNum[64] = { 0 };

    YMLog("");
    errcount = YMGetFaultNum(errNum, YmTrue);
    YMProtoLoginNotice((int)net->nm, YMGetReportInterval(), errNum, errcount);
}

#if !YM_OPTION_TCP_CONNECT_SUPPORT_URL
_funcTag static void dnsResult(YMSocket_t *nm, const char *host, YmBool success)
{
    YMServerNetwork_t *net = (YMServerNetwork_t *)nm->private;

    YMLog("success:%d", success);
    if(net && success)
    {
        YMLog("%s -> %s", net->serverUrl, host);
        YMSocketTcpConnect(nm, host, net->serverPort);
    }
}

_funcTag static void serverUrlDns(YMServerNetwork_t *net)
{
    YMSocketDNSResolve(net->nm, net->serverUrl, dnsResult);
}
#endif

_funcTag static void tcpDisconnetCb(YMSocket_t *nm)
{
    YMProtoClearSendList((int)nm);
    serverStatusChange((YMServerNetwork_t *)nm->private, YM_SERVER_LINK_DISCONNECTED);
}

_funcTag static void tcpConnectCb(YMSocket_t *nm, YmBool success)
{
    YMLog("[%p] connect result [%d]", nm, success);

    if(success)
    {
        serverStatusChange((YMServerNetwork_t *)nm->private, YM_SERVER_LINK_CONNECTED);
    }
    else
    {
        YMProtoClearSendList((int)nm);
        serverStatusChange((YMServerNetwork_t *)nm->private, YM_SERVER_LINK_DISCONNECTED);
    }
}

_funcTag static void tcpRecvCb(YMSocket_t *nm, YmUint8_t *data, YmUint16_t len)
{
    YMProtoMessageRecv((int)nm, (char *)data, len);
}

_funcTag static void tcpSendFailedCb(YMSocket_t *nm, YmUint8_t *data, YmUint16_t len)
{
#if YM_RESUME_TRANSFER_ENABLE
    char *param = YMProtoGetCommandParam((char *)data);

    if(param)
    {
        YMLog("RETRANS_SAVE:%s", param);
        YM_RETRANS_SAVE_LOST_DATA(((YMServerNetwork_t *)nm->private)->serverID, param);
        free(param);
    }
#endif

}

_funcTag const char *YMServerGetPhyUID(void)
{
    return YMSocketGetPhyUID();
}

_funcTag void YMServerStop(void)
{
    YMLog("");
    YMSocketStop();
    g_serverStart = YmFalse;
}

_funcTag void YMServerStart(void)
{
    YMLog("");
    YMSocketStart();
    g_serverStart = YmTrue;
}

_funcTag static void serverLoginManager(void)
{
    YmUint8_t i;

    for(i = 0; i < YM_SERVER_LINK_NUM; i++)
    {
        if(g_serverNet[i].serverStatus == YM_SERVER_LINK_CONNECTED)
        {
            if(g_serverNet[i].retryCount > 5)
            {
                //serverStatusChange(&g_serverNet[i], YM_SERVER_LINK_ABNORMAL);
                serverEventEmit(YMSERVER_EVENT_LINK_ABNORMAL, NULL, 0);
                g_serverNet[i].retryCount = 0;
            }
            else
            {
                if(g_serverNet[i].lastActiveTime == 0 || YMSysTimeHasPast(g_serverNet[i].lastActiveTime, 60000))
                {
                    loginServer(&g_serverNet[i]);
                    g_serverNet[i].lastActiveTime = YMSysTime();
                    g_serverNet[i].retryCount++;
                }
            }
        }
        else if(g_serverNet[i].serverStatus == YM_SERVER_LINK_DISCONNECTED)
        {
            if(YMSocketPhyLinked() && YMSysTimeHasPast(g_serverNet[i].lastActiveTime, 30000))
            {
                if(g_serverNet[i].retryCount > 10)
                {
                    //serverStatusChange(&g_serverNet[i], YM_SERVER_LINK_ABNORMAL);
                    serverEventEmit(YMSERVER_EVENT_LINK_ABNORMAL, NULL, 0);
                    g_serverNet[i].retryCount = 0;
                }
                else
                {
#if YM_OPTION_TCP_CONNECT_SUPPORT_URL
                    YMSocketTcpConnect(g_serverNet[i].nm, g_serverNet[i].serverUrl, g_serverNet[i].serverPort);
#else
                    serverUrlDns(&g_serverNet[i]);
#endif
                    g_serverNet[i].lastActiveTime = YMSysTime();
                    g_serverNet[i].retryCount++;
                }
            }
        }
        else
        {
        }
    }
}

_funcTag static void networkInit(void)
{
    YMServerNetwork_t *net;

    net = &g_serverNet[YM_SERVER_ID_MASTER];
    net->serverID = YM_SERVER_ID_MASTER;
    net->serverUrl = YM_MASTER_SERVER_URL;
    net->serverPort = YM_MASTER_SERVER_PORT;
    net->serverStatus = YM_SERVER_LINK_DISCONNECTED;
    net->nm = YMSocketCreate();
    YMLog("socket[0] create :%p", net->nm);

    if(net->nm)
    {
        net->nm->connCb     = tcpConnectCb;
        net->nm->recvCb     = tcpRecvCb;
        net->nm->disconnCb  = tcpDisconnetCb;
        net->nm->sendFailCb = tcpSendFailedCb;
        net->nm->private    = net;
    }

#if YM_SLAVE_SERVER_ENABLE
    net = &g_serverNet[YM_SERVER_ID_SLAVE];
    net->serverID = YM_SERVER_ID_SLAVE;
    net->serverUrl = YM_SLAVE_SERVER_URL;
    net->serverPort = YM_SLAVE_SERVER_PORT;
    net->serverStatus = YM_SERVER_LINK_DISCONNECTED;
    net->nm = YMSocketCreate();
    YMLog("socket[1] create :%p", net->nm);

    if(net->nm)
    {
        net->nm->connCb     = tcpConnectCb;
        net->nm->recvCb     = tcpRecvCb;
        net->nm->disconnCb  = tcpDisconnetCb;
        net->nm->sendFailCb = tcpSendFailedCb;
        net->nm->private    = net;
    }
#endif
}

_funcTag void YMServerPoll(void)
{
    if(g_serverStart)
    {
        faultCheck();
        serverReplyPoll();
        serverLoginManager();
        YMOTAPoll();
        YMProtoPoll();
#if YM_RESUME_TRANSFER_ENABLE
        breakpointResume();
#endif
    }
    YMSocketPoll();
}

_funcTag void YMServerInitialize(void)
{
    YMListInit(&g_serverReply);
    YMSocketInitialize();
    YMOTAInitialize();
    YMOTAEventRegister(otaEvenetHandle);
    YMProtoInitialize();
    YMProtoCallbackRegister(serverDataSend, protocolEventHandle);
    networkInit();
#if YM_RESUME_TRANSFER_ENABLE
    YM_RETRANS_UPDATE_INFO();
#endif
}

