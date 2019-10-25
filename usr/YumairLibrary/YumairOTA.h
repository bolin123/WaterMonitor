#ifndef YUMAIR_OTA_H
#define YUMAIR_OTA_H

#include "YMPrivate.h"

typedef enum
{
    YMOTA_EVENT_START,
    YMOTA_EVENT_PROGRESS,
    YMOTA_EVENT_SUCCESS,
    YMOTA_EVENT_FAILED,
}YMOTAEvent_t;

typedef void (* YMOTAEventHandle_cb)(YMOTAEvent_t event, void *args, YmUint32_t replyID);

void YMOTAEventRegister(YMOTAEventHandle_cb eventHandle);
void YMOTAPoll(void);
int YMOTAStart(const char *url, YmUint32_t size, YmUint8_t *md5, YmUint8_t *version, YmUint32_t replyID);
void YMOTAInitialize(void);
#endif

