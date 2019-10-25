#include "YumairOTA.h"
#include "HTTPRequest.h"
#include "MD5.h"

static YMOTAInfo_t g_otaInfo;
static HTTPRequest_t *g_http = YmNULL;
static YMOTAEventHandle_cb g_eventHandle = YmNULL;
static YmUint32_t g_replyID;
static YmBool g_start = YmFalse;

_funcTag static void otaEventEmit(YMOTAEvent_t event, void *args, YmUint32_t replyID)
{
    if(g_eventHandle)
    {
        g_eventHandle(event, args, replyID);
    }
}

_funcTag static YmBool md5Checked(void)
{
    MD5_CTX ctx;
    YmUint8_t data[512];
    YmUint16_t count, i, lastSize;
    YmUint32_t flashOffset = 0;
    YmUint8_t md5[16];
    
    MD5Init(&ctx);

    count = g_otaInfo.size / sizeof(data);
    lastSize = g_otaInfo.size % sizeof(data);

    for(i = 0; i < count; i++)
    {
        YM_OTA_DATA_READ(flashOffset, data, sizeof(data));
        MD5Update(&ctx, data, sizeof(data));
        flashOffset += sizeof(data);
    }
    if(lastSize)
    {
        YM_OTA_DATA_READ(flashOffset, data, lastSize);
        MD5Update(&ctx, data, lastSize);
        flashOffset += lastSize;
    }

    MD5Final(&ctx, md5);

    if(memcmp(md5, g_otaInfo.md5, 16) == 0)
    {
        YMLog("md5 check ok");
        return YmTrue;
    }

    YMLog("md5 check error!");
    return YmFalse;
}

_funcTag static void httpRequestCallback(HTTPRequest_t *request, const YmUint8_t *data, YmUint16_t len, HTTPRequestError_t error)
{
    int progress;
    static YmUint32_t flashOffset = 0;
    
    if(error == HTTP_REQ_ERROR_NONE)
    {
        YM_OTA_DATA_WRITE(flashOffset, (YmUint8_t *)data, len);
        flashOffset += len;
        progress = (flashOffset * 100) / g_otaInfo.size;
        otaEventEmit(YMOTA_EVENT_PROGRESS,(void *)progress, g_replyID);
    }
    else if(error == HTTP_REQ_ERROR_SUCCESS)
    {
        if(g_otaInfo.size == request->respContentLength)
        {
            if(md5Checked())
            {
                otaEventEmit(YMOTA_EVENT_SUCCESS, (void *)&g_otaInfo, g_replyID);
            }
            else
            {
                otaEventEmit(YMOTA_EVENT_FAILED, (void *)&g_otaInfo, g_replyID);
            }
        }
        
        HTTPRequestDestroy(g_http);
        g_http = YmNULL;
        g_start = YmFalse;
    }
    else
    {
        otaEventEmit(YMOTA_EVENT_FAILED, (void *)&g_otaInfo, g_replyID);
        HTTPRequestDestroy(g_http);
        g_http = YmNULL;
        g_start = YmFalse;
    }
}


_funcTag void YMOTAEventRegister(YMOTAEventHandle_cb eventHandle)
{
    g_eventHandle = eventHandle;
}

_funcTag void YMOTAPoll(void)
{
    HTTPRequestPoll();
}

_funcTag int YMOTAStart(const char *url, YmUint32_t size, YmUint8_t *md5, YmUint8_t *version, YmUint32_t replyID)
{
    if(g_start)
    {
        return -1;
    }

    YMLog("url:%s, size:%d, version:%d.%d.%d.%d", url, size, \
        version[0], version[1], version[2], version[3]);
    
    g_start = YmTrue;
    g_otaInfo.size = size;
    memcpy(g_otaInfo.md5, md5, 16);
    memcpy(g_otaInfo.version, version, 4);
    g_replyID = replyID;
    
    YM_OTA_SECTIONS_ERASE(size);
    if(g_http != YmNULL)
    {
        HTTPRequestDestroy(g_http);
        g_http = YmNULL;
    }
    g_http = HTTPRequestCreate(url, HTTP_REQ_METHOD_GET);
    g_http->dataRecvCb = httpRequestCallback;
    HTTPRequestStart(g_http);
    otaEventEmit(YMOTA_EVENT_START, &g_otaInfo, g_replyID);
    return 0;
}

_funcTag void YMOTAInitialize(void)
{
}


