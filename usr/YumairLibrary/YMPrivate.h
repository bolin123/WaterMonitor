#ifndef YM_PRIVATE_H
#define YM_PRIVATE_H

#include "YMCtype.h"
#include "Yumair.h"

const char *YMServerGetPhyUID(void);

const YmUint8_t *YMGetFirmVersion(void);
const char *YMGetDeviceModel(void);
const char *YMGetDevPasswd(void);
const char *YMGetDevID(void);
YmUint16_t YMGetReportInterval(void);
YmUint8_t YMGetSleepMode(void);
YmUint8_t YMGetFaultNum(YmUint8_t *fbuff, YmBool force);
YMLocationInfo_t *YMGetLocationInfo(void);

#endif // !YM_PRIVATE_H

