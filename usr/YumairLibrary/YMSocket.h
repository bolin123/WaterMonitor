#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "YMCtype.h"

/*Socket结构体
* 用于创建TCP 网络连接
*/
typedef struct YMSocket_st
{
    void (*connCb)(struct YMSocket_st *handle, YmBool success); //tcp connect 回调函数
    void (*recvCb)(struct YMSocket_st *handle, YmUint8_t *data, YmUint16_t length); //接收 回调函数
    void (*disconnCb)(struct YMSocket_st *handle); //断开回调函数
    void (*sendFailCb)(struct YMSocket_st *handle, YmUint8_t *data, YmUint16_t length);//发送失败回调(可选)
    void *socket; //底层socket
    void *private; //备用私有指针
}YMSocket_t;

/*DNS解析回调函数指针
*@host:解析的结果(失败为YmNULL)，@success:是否成功
*/
typedef void (*YMSocketDnsResult_cb)(YMSocket_t *handle, const char *host, YmBool success);

/*获取PHY唯一码
* GPRS返回ICCID
*/
const char *YMSocketGetPhyUID(void);

/*PHY网络连接情况
*返回值:是否连接到以太网
*/
YmBool YMSocketPhyLinked(void);

/*DNS解析函数
*@handle:YMSocket句柄,url:域名,@resultCb:解析结果回调函数
*/
void YMSocketDNSResolve(YMSocket_t *handle, const char *url, YMSocketDnsResult_cb resultCb);

/*TCP close
*@handle:YMSocket句柄
*/
void YMSocketTcpClose(YMSocket_t *handle);

/*TCP send
*@handle:YMSocket句柄,@data:数据,@length:数据长度
*/
int YMSocketTcpSend(YMSocket_t *handle, const YmUint8_t *data, YmUint16_t length);

/*TCP connect
@handle:YMSocket句柄,@host:IP或域名,@port:端口号
*/
int YMSocketTcpConnect(YMSocket_t *handle, const char *host, YmUint16_t port);

/*释放
*/
void YMSocketRelease(YMSocket_t *handle);

/*创建
*/
YMSocket_t *YMSocketCreate(void);

/*停止
*/
void YMSocketStop(void);

/*开始
*/
void YMSocketStart(void);

/*初始化
*/
void YMSocketInitialize(void);

/*轮询
*/
void YMSocketPoll(void);

#endif

