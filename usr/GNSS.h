#ifndef GNSS_H
#define GNSS_H

#include "Sys.h"

#define GNSS_FIX_TIME_MAX 180000

/*��λ��ʱ����Ϣ*/
typedef struct
{
    struct YMDateTime_st time;
    struct
    {
        float latitude;
        float longitude;
    }location;
}GNSSLocation_t;


typedef enum
{
    GEVENT_GNSS_START,      //GPS����
    GEVENT_GNSS_STOP,       //GPS�ر�
    GEVENT_GNSS_FIXED,      //GPS������Ϣ
    GEVENT_GNSS_TIMEOUT,
}GNSSEvent_t;

typedef void (* GNSSEventHandle_cb)(GNSSEvent_t event, void *args);

/*�ر�GPS
* @return: 0=success, -1=fail
*/
int8_t GNSSStop(void);

/*����GPS
* @return: 0=success, -1=fail
*/
int8_t GNSSStart(void);

/*GPS�Ƿ�����*/
bool GNSSLocationFixed(void);

/*��ȡ��λ��ʱ����Ϣ*/
GNSSLocation_t *GNSSGetLocation(void);

void GNSSEventHandleRegister(GNSSEventHandle_cb handle);


#endif

