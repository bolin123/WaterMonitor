#ifndef NET_MANAGER_H
#define NET_MANAGER_H

#include "YMCtype.h"

/*Socket�ṹ��
* ���ڴ���TCP ��������
*/
typedef struct YMSocket_st
{
    void (*connCb)(struct YMSocket_st *handle, YmBool success); //tcp connect �ص�����
    void (*recvCb)(struct YMSocket_st *handle, YmUint8_t *data, YmUint16_t length); //���� �ص�����
    void (*disconnCb)(struct YMSocket_st *handle); //�Ͽ��ص�����
    void (*sendFailCb)(struct YMSocket_st *handle, YmUint8_t *data, YmUint16_t length);//����ʧ�ܻص�(��ѡ)
    void *socket; //�ײ�socket
    void *private; //����˽��ָ��
}YMSocket_t;

/*DNS�����ص�����ָ��
*@host:�����Ľ��(ʧ��ΪYmNULL)��@success:�Ƿ�ɹ�
*/
typedef void (*YMSocketDnsResult_cb)(YMSocket_t *handle, const char *host, YmBool success);

/*��ȡPHYΨһ��
* GPRS����ICCID
*/
const char *YMSocketGetPhyUID(void);

/*PHY�����������
*����ֵ:�Ƿ����ӵ���̫��
*/
YmBool YMSocketPhyLinked(void);

/*DNS��������
*@handle:YMSocket���,url:����,@resultCb:��������ص�����
*/
void YMSocketDNSResolve(YMSocket_t *handle, const char *url, YMSocketDnsResult_cb resultCb);

/*TCP close
*@handle:YMSocket���
*/
void YMSocketTcpClose(YMSocket_t *handle);

/*TCP send
*@handle:YMSocket���,@data:����,@length:���ݳ���
*/
int YMSocketTcpSend(YMSocket_t *handle, const YmUint8_t *data, YmUint16_t length);

/*TCP connect
@handle:YMSocket���,@host:IP������,@port:�˿ں�
*/
int YMSocketTcpConnect(YMSocket_t *handle, const char *host, YmUint16_t port);

/*�ͷ�
*/
void YMSocketRelease(YMSocket_t *handle);

/*����
*/
YMSocket_t *YMSocketCreate(void);

/*ֹͣ
*/
void YMSocketStop(void);

/*��ʼ
*/
void YMSocketStart(void);

/*��ʼ��
*/
void YMSocketInitialize(void);

/*��ѯ
*/
void YMSocketPoll(void);

#endif

