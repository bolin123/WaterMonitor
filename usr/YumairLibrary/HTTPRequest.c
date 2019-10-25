#include "HTTPRequest.h"
#include "YMSocket.h"

#define CONTENT_LENGTH_FLAG "Content-Length: "

static HTTPRequest_t *g_httpRequest = YmNULL;
static YMSocket_t *g_net = YmNULL;

_funcTag static void handleEnd(HTTPRequest_t *request, YmBool success)
{
    request->hasStart = YmFalse;
    YMSocketTcpClose(g_net);
    if(request->dataRecvCb)
    {
        request->dataRecvCb(request, YmNULL, 0, success ? HTTP_REQ_ERROR_SUCCESS : HTTP_REQ_ERROR_FAIL);
    }
}

_funcTag static YmBool parseRequestURL(HTTPRequest_t *request)
{
    char *p;

    const char *hostStart = request->url;
    p = strstr(hostStart, "/");
    if(!p)
    {
        p = request->url + strlen(request->url);
    }

    char host[300] = {0};
    char port[10] = "80";
    memcpy(host, hostStart, (int)(p - hostStart));

    p = strchr(host, ':');
    if(p)
    {
        p[0] = 0;
        p++;
        strcpy(port, p);
    }

    request->host = YmMalloc(strlen(host) + 1);
    if(request->host)
    {
        strcpy(request->host, host);
    }
    request->port = atoi(port);
    YMLog("host %s:%s", request->host, port);

    return YmTrue;
}

_funcTag HTTPRequest_t *HTTPRequestCreate(const char *url, HTTPRequestMethod_t method)
{
    HTTPRequest_t *request = YmMalloc(sizeof(HTTPRequest_t));
    YMLog("request:%p", request);

    request->method = method;
    request->url = YmMalloc(strlen(url) + 1);
	request->headerBuf = YmMalloc(512);

    if(strstr(url, "http://"))
    {
        strcpy(request->url, url + 7);
    }
    else
    {
        strcpy(request->url, url);
    }

    YMListInit(&request->params);
    request->host = YmNULL;
    request->dataRecvCb = YmNULL;
    request->data = YmNULL;
    request->hasStart =YmFalse;
    request->startConnect = YmFalse;

    if(parseRequestURL(request))
    {
        YMLog("");
        return request;
    }
    else
    {
        HTTPRequestDestroy(request);
        return YmNULL;
    }
}

_funcTag static void destroyParam(HTTPParam_t *param)
{
    free(param->key);
    free(param->value);
    free(param);
}

_funcTag void HTTPRequestDestroy(HTTPRequest_t *request)
{
    free(request->url);
    YMLog("free(request->url)");

    YMSocketRelease(g_net);
    g_net = YmNULL;

    HTTPParam_t *param;
    HTTPParam_t *lastParam = YmNULL;

	free(request->headerBuf);

    if(request->data)
    {
        free(request->data);
    }

    YMListForeach(&request->params, param)
    {
        if(lastParam)
        {
            destroyParam(lastParam);
        }
        lastParam = param;
    }
    if(lastParam)
    {
        destroyParam(lastParam);
    }

    YMLog("destroyParam(param)");


    if(request->host)
    {
        free(request->host);
        request->host = YmNULL;
    }

    YMLog("free(request->host)");


    free(request);
    g_httpRequest = YmNULL;
}

_funcTag void HTTPRequestAddParam(HTTPRequest_t *request, const char *key, const char *value)
{
    HTTPParam_t *param = YmMalloc(sizeof(HTTPParam_t));
    param->key = YmMalloc(strlen(key) + 1);
    param->value = YmMalloc(strlen(value) + 1);
    strcpy(param->key, key);
    strcpy(param->value, value);
    YMListAdd(&request->params, param);
}

_funcTag static const char *methodToStr(HTTPRequestMethod_t method)
{
    switch(method)
    {
        case HTTP_REQ_METHOD_POST:
            return "POST";

        case HTTP_REQ_METHOD_GET:
            return "GET";
    }
		return "POST";
}

_funcTag static void connectCb(struct YMSocket_st *nm, YmBool success)
{
    HTTPRequest_t *request = g_httpRequest;

    YMLog("req:%s result:%d", request->url, success);

    if(!success)
    {
        //handleEnd(request, YmFalse);
        return;
    }
    
    request->startConnect = YmFalse;
    request->rnCount = 0;
    request->headerBufCount = 0;
    request->respContentLength = 0;

    char reqData[300] = {0};

    char *path = strstr(request->url, "/");
    if(path == YmNULL)
    {
        path = "/";
    }
    sprintf(reqData, "%s %s", methodToStr(request->method), strstr(request->url, "/"));
    strcat(reqData, " HTTP/1.1\r\n");
    strcat(reqData, "Host: ");
    strcat(reqData, request->host);

    if(request->port != 80)
    {
        char tmp[10];
        strcat(reqData, ":");
        sprintf(tmp, "%d", request->port);
        strcat(reqData, tmp);
    }
    strcat(reqData, "\r\n");
    strcat(reqData, "Connection: Keep-Alive\r\n");

    strcat(reqData, "Content-Type: application/x-www-form-urlencoded\r\n");
    YmUint16_t contentLen = 0;

    YmBool hasParamBefore = YmFalse;;
    HTTPParam_t *param;

    //计算 Content-Length
    //带数据
    if(request->data)
    {
        contentLen = strlen(request->data);
    }
    //带参数
    else
    {
        YMListForeach(&request->params, param)
        {
            if(hasParamBefore)
            {
                contentLen++;
            }

            contentLen = contentLen + strlen(param->key) + strlen(param->value) + 1;
            hasParamBefore = YmTrue;
        }
    }

    sprintf(reqData + strlen(reqData), "Content-Length: %d\r\n", contentLen);
    strcat(reqData, "\r\n");

    if(request->data)
    {
        strcat(reqData, request->data);
    }
    else
    {
        hasParamBefore = YmFalse;
        YMListForeach(&request->params, param)
        {
            if(hasParamBefore)
            {
                strcat(reqData, "&");
            }

            strcat(reqData, param->key);
            strcat(reqData, "=");
            strcat(reqData, param->value);
            hasParamBefore = YmTrue;
        }
    }

    YMLog("request\n%s", reqData);

    YMSocketTcpSend(nm, (YmUint8_t *)reqData, strlen(reqData));
    request->hasStart = YmTrue;
    request->validTime = YMSysTime();
}

_funcTag static void disconnectCb(struct YMSocket_st *nm)
{
    if(g_httpRequest)
    {
        YMLog("req:%s", g_httpRequest->url);
        handleEnd(g_httpRequest, YmFalse);
    }
}


#define CHAR_DIGIT(ch) ((ch) - '0' < 10 ? (ch) - '0' : (ch) - 'a' + 10)

_funcTag static YmUint32_t parseHexNumStr(const char *str)
{
    YmUint8_t len = strlen(str);
    YmUint8_t i;
    YmUint32_t num = 0;
    char ch;
    for(i = 0; i < len; i++)
    {
        ch = str[i];
        num <<= 4;

        //转小写
        if(ch >= 'A' && ch <= 'Z')
        {
            ch += ('a' - 'A');
        }

        num |= CHAR_DIGIT(ch);
    }
    return num;
}

_funcTag static void recvCb(struct YMSocket_st *nm, YmUint8_t *data, YmUint16_t len)
{
    HTTPRequest_t *request = g_httpRequest;

    YmUint32_t i;
//    YMLog("");

    request->validTime = YMSysTime();

    //\r\n\r\n表示HTTP头结束
    //接收header中
    if(request->rnCount < 4)
    {
        for(i = 0; i < len; i++)
        {
            request->headerBuf[request->headerBufCount++] = data[i];
            request->headerBuf[request->headerBufCount] = 0;

            if(data[i] == '\r' || data[i] == '\n')
            {
                request->rnCount++;
            }
            else
            {
                request->rnCount = 0;
            }

            if(request->rnCount == 4)
            {
//                YMLog("%s recv header:%s", request->url, request->headerBuf);

                char *p = strstr((char *)request->headerBuf, CONTENT_LENGTH_FLAG);
                if(p)
                {
                    p += strlen(CONTENT_LENGTH_FLAG);
                    char lenStr[10] = {0};
                    char *p2 = strchr(p, '\r');
                    memcpy(lenStr, p, p2 - p);
                    request->respContentLength = atoi(lenStr);
                    request->respContentDataCount = 0;
                    YMLog("respContentLength %d", request->respContentLength);
                }
                else
                {
                    request->chunkLen = 0;
                    request->chunkRNCount = 0;
                    request->headerBufCount = 0;
                }

                data = data + i + 1;
                len = len - i - 1;
                break;
            }
        }
    }

    if(request->rnCount < 4)
    {
        return;
    }

    //指定Content-Length
    if(request->respContentLength != 0)
    {
//        YMLog("len:%d count:%d contentLen:%d", len, request->respContentDataCount, request->respContentLength);
        request->respContentDataCount += len;

        if(request->dataRecvCb)
        {
            request->dataRecvCb(request, data, len, HTTP_REQ_ERROR_NONE);
        }

        if(request->respContentDataCount >= request->respContentLength)
        {
            YMLog("success.");
            handleEnd(request, YmTrue);
        }
    }
    //transfer-encoding:chunked，chunk格式:[hex]\r\n[Data]\r\n[...]0\r\n
    else
    {
        for(i = 0; i < len; i++)
        {
//            YMPrint("%c", data[i]);

            //获取chunk长度
            if(request->chunkLen == 0)
            {
                if(data[i] == '\r' || data[i] == '\n')
                {
                    request->chunkRNCount++;
                }
                else
                {
                    request->chunkRNCount = 0;
                    request->headerBuf[request->headerBufCount++] = data[i];
                    request->headerBuf[request->headerBufCount] = '\0';
                }

                if(request->chunkRNCount == 2)
                {
                    request->chunkRNCount = 0;
                    request->headerBufCount = 0;
                    request->chunkLen = parseHexNumStr(request->headerBuf);
                    YMLog("chunk len:%d[%s]", request->chunkLen, request->headerBuf);
                    if(request->chunkLen == 0)
                    {
                        handleEnd(request, YmTrue);
                        return;
                    }
                    else
                    {
                        //chunk结尾的\r\n
                        request->chunkLen += 2;
                    }
                }
            }
            else
            {
                //屏蔽chunk结尾的\r\n
                if(request->chunkLen > 2)
                {
                    request->dataRecvCb(request, &data[i], 1, HTTP_REQ_ERROR_NONE);
                }
                request->chunkLen--;
            }
        }
    }
}


_funcTag void HTTPRequestSetData(HTTPRequest_t *request, const char *data)
{
    if(request->data)
    {
        free(request->data);
    }
    request->data = YmMalloc(strlen(data) + 1);
    strcpy(request->data, data);
}

_funcTag void HTTPRequestStart(HTTPRequest_t *request)
{
    g_httpRequest = request;

    YMLog("start connect:%s", request->url);

    if(g_net == YmNULL)
    {
        g_net = YMSocketCreate();
    }
    
    if(g_net)
    {
        g_net->connCb = connectCb;
        g_net->recvCb = recvCb;
        g_net->disconnCb = disconnectCb;

        YMSocketTcpConnect(g_net, request->host, request->port);
        request->connectTime = YMSysTime();
        request->reconnectTime = 1;
        request->startConnect = YmTrue;
    }
    
}

_funcTag void HTTPRequestPoll(void)
{
    HTTPRequest_t *request = g_httpRequest;
    //超时

    if(request != YmNULL
        && request->hasStart 
        && YMSysTimeHasPast(request->validTime, 60000))
    {
        YMLog("http request timeout");
        handleEnd(request, YmFalse);
    }

    if(request != YmNULL && request->startConnect 
        && YMSysTimeHasPast(request->connectTime, 10000))
    {
        if(request->reconnectTime >= 3)
        {
            request->startConnect = YmFalse;
            YMLog("reconnect failed!");
            handleEnd(request, YmFalse);
        }
        else
        {   YMLog("reconnect ...");
            YMSocketTcpConnect(g_net, request->host, request->port);
            request->connectTime = YMSysTime();
            request->reconnectTime++;
        }
    }
    
}


