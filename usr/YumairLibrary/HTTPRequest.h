#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "YMCtype.h"
#include "YMList.h"

typedef enum
{
    HTTP_REQ_METHOD_POST,
    HTTP_REQ_METHOD_GET,
}HTTPRequestMethod_t;

typedef enum
{
    HTTP_REQ_ERROR_NONE = 0,
    HTTP_REQ_ERROR_FAIL,
    HTTP_REQ_ERROR_SUCCESS,
}HTTPRequestError_t;

typedef struct HTTPParam_st
{
    char *key;
    char *value;
    YMLIST_ENTRY(struct HTTPParam_st);
}HTTPParam_t;

typedef struct HTTPRequest_st HTTPRequest_t;

typedef void (*HTTPRequestDataRecvCallback_t)(HTTPRequest_t *request, const YmUint8_t *data, YmUint16_t len, HTTPRequestError_t error);


struct HTTPRequest_st
{
    char *url;
    char *host;
    YmUint16_t port;

    HTTPRequestMethod_t method;
    HTTPParam_t params;
    char *data;

    //
    HTTPRequestDataRecvCallback_t dataRecvCb;
    YmBool hostIsDomain : 1;

    void *userData;

    YmBool hasStart;
    YmTime_t validTime;

    YmBool startConnect;
    YmUint8_t reconnectTime;
    YmTime_t connectTime;
    //
    char *headerBuf;
    YmUint16_t headerBufCount;
    YmUint8_t rnCount;

    //
    YmUint16_t chunkLen;
    YmUint8_t chunkRNCount;


    YmUint32_t respContentDataCount;
    YmUint32_t respContentLength;

    YMLIST_ENTRY(struct HTTPRequest_st);
};



HTTPRequest_t *HTTPRequestCreate(const char *url, HTTPRequestMethod_t method);

void HTTPRequestDestroy(HTTPRequest_t *request);

void HTTPRequestSetData(HTTPRequest_t *request, const char *data);
void HTTPRequestAddParam(HTTPRequest_t *request, const char *key, const char *value);
void HTTPRequestStart(HTTPRequest_t *request);
void HTTPRequestPoll(void);

#endif // HTTP_REQUEST_H



