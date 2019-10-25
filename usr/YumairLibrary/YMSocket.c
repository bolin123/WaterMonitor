#include "YMSocket.h"
#include "GPRS.h"
#include "GPRSPower.h"

#define NET_MANAGER_MAX_NUM 3
#define NET_MANAGER_CONNECT_INTERVAL 20000

static YmUint8_t g_nmCount = 0;
static YMSocket_t *g_nm[NET_MANAGER_MAX_NUM];

_funcTag static YMSocket_t *getManager(gprsSocketfd socketfd)
{
    YmUint8_t i;
    GPRSTcpSocket_t *socket;

    for(i = 0; i < g_nmCount; i++)
    {
        if(g_nm[i])
        {
            socket = (GPRSTcpSocket_t *)g_nm[i]->socket;
            if(socket && socket->socketId == socketfd)
            {
                return g_nm[i];
            }
        }
    }
    return YmNULL;
}

_funcTag static void nmDataSendFailed(gprsSocketfd socketfd, YmUint8_t *data, YmUint16_t length)
{
    YMSocket_t *nm = getManager(socketfd);
    
    if(nm && nm->sendFailCb)
    {
        nm->sendFailCb(nm, data, length);
    }
}

_funcTag static void nmDataReceive(gprsSocketfd socketfd, YmUint8_t *data, YmUint16_t length)
{
    YMSocket_t *nm = getManager(socketfd);
    
    if(nm && nm->recvCb)
    {
        nm->recvCb(nm, data, length);
    }
}

_funcTag static void nmDisconnect(gprsSocketfd socketfd)
{
    YMSocket_t *nm = getManager(socketfd);
    
    if(nm && nm->disconnCb)
    {
        nm->disconnCb(nm);
    }
}

_funcTag static void nmConnect(gprsSocketfd socketfd, YmBool success)
{
    YMSocket_t *nm = getManager(socketfd);
    
    if(nm && nm->connCb)
    {   
        nm->connCb(nm, success);
    }
}

_funcTag void YMSocketDNSResolve(YMSocket_t *manager, const char *url, YMSocketDnsResult_cb resultCb)
{
    if(manager)
    {
        resultCb(manager, "123.59.89.235", YmTrue);
    }
}

_funcTag void YMSocketTcpClose(YMSocket_t *manager)
{
    if(manager)
    {
        GPRSTcpClose((GPRSTcpSocket_t *)manager->socket);
    }
}

_funcTag YmBool YMSocketPhyLinked(void)
{
    return GPRSConnected();
}

_funcTag int YMSocketTcpSend(YMSocket_t *manager, const YmUint8_t *data, YmUint16_t length)
{
    if(manager && YMSocketPhyLinked())
    {
        return GPRSTcpSend((GPRSTcpSocket_t *)manager->socket, (YmUint8_t *)data, length);
    }
    return -1;
}

_funcTag int YMSocketTcpConnect(YMSocket_t *manager, const char *host, YmUint16_t port)
{
    if(manager)
    {
        return GPRSTcpConnect((GPRSTcpSocket_t *)manager->socket, host, port);
    }
    return -1;
}

_funcTag void YMSocketRelease(YMSocket_t *nm)
{
    if(nm)
    {
        if(nm->socket)
        {
            GPRSTcpRelease((GPRSTcpSocket_t *)nm->socket);
        }
        free(nm);
    }
}

_funcTag YMSocket_t *YMSocketCreate(void)
{
    YMSocket_t *manager = (YMSocket_t *)YmMalloc(sizeof(YMSocket_t));

    if(manager)
    {
        manager->socket = (void *)GPRSTcpCreate();
        if(manager->socket)
        {   
            GPRSTcpSocket_t *gsocket = (GPRSTcpSocket_t *)manager->socket;
            gsocket->connectCb = nmConnect;
            gsocket->disconnetCb = nmDisconnect;
            gsocket->recvCb = nmDataReceive;
            gsocket->sendFailCb = nmDataSendFailed;
            g_nm[g_nmCount++] = manager;
            return manager;
        }
        else
        {
            free(manager);
            return YmNULL;
        }
    }
    
    return YmNULL;
}

_funcTag const char *YMSocketGetPhyUID(void)
{
    return GPRSGetICCID();
}

_funcTag void YMSocketStart(void)
{
    GPRSPowerOn();
}

_funcTag void YMSocketStop(void)
{
    GPRSPowerOff();
}

_funcTag void YMSocketInitialize(void)
{
    GPRSInitialize();
    GPRSPowerInitialize();
}

_funcTag void YMSocketPoll(void)
{
    GPRSPoll();
    GPRSPowerPoll();
}

