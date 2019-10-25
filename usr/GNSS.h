#ifndef GNSS_H
#define GNSS_H

#include "Sys.h"

#define GNSS_FIX_TIME_MAX 180000

/*定位与时间信息*/
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
    GEVENT_GNSS_START,      //GPS开启
    GEVENT_GNSS_STOP,       //GPS关闭
    GEVENT_GNSS_FIXED,      //GPS锁定信息
    GEVENT_GNSS_TIMEOUT,
}GNSSEvent_t;

typedef void (* GNSSEventHandle_cb)(GNSSEvent_t event, void *args);

/*关闭GPS
* @return: 0=success, -1=fail
*/
int8_t GNSSStop(void);

/*开启GPS
* @return: 0=success, -1=fail
*/
int8_t GNSSStart(void);

/*GPS是否锁定*/
bool GNSSLocationFixed(void);

/*获取定位和时间信息*/
GNSSLocation_t *GNSSGetLocation(void);

void GNSSEventHandleRegister(GNSSEventHandle_cb handle);


#endif

