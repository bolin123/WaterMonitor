#ifndef SYS_H
#define SYS_H

#include "HalCommon.h"
#include "Yumair.h"

//#define SYS_GPRS_COMM_PORT   HAL_UART_PORT_1
//#define SYS_SENSOR_COMM_PORT HAL_UART_PORT_2
//#define SYS_LOGS_COMM_PORT   HAL_UART_PORT_3

#define SYS_DEVICE_ID_LEN 16
#define SYS_PASSWORD_LEN 8

#define SYS_SAVE_LOST_MSG_NUM_MAX 60 //断点续传最大个数

#define DEBUG_ENABLE 1
#define SysTime_t uint32_t
#define SysTime HalGetSysTimeCount
#define SysTimeHasPast(oldtime, pass) ((SysTime() - (oldtime)) > pass)

#if DEBUG_ENABLE

#define HalPrintf(...) printf(__VA_ARGS__)
#define SysPrintf HalPrintf
#define SysLog(...) SysPrintf("%s %s: ", SysPrintDateTime(), __FUNCTION__); SysPrintf(__VA_ARGS__); SysPrintf("\n");
#else
#define HalPrintf(...)
#define SysPrintf HalPrintf
#define SysLog(...)
#endif

typedef enum
{
    SYS_SLEEP_MODE_NONE = 0,
    SYS_SLEEP_MODE_DEEP,  //深度休眠
    SYS_SLEEP_MODE_LIGHT, //浅休眠
}SysSleepMode_t;

typedef struct
{
    bool fixed;
    float latitude;
    float longitude;
}SysLocation_t;

void *SysMalloc(uint16_t size);

//休眠相关
void SysSleep(void);
bool SysNeedSleep(void);
void SysSleepResume(void);
bool SysIsRtcTimeup(void);
void SysRtcTimeup(bool set);
HalBootMode_t SysBootMode(void);
void SysSetSleepMode(uint8_t mode);
SysSleepMode_t SysGetSleepMode(void);

void SysRetransInit(void);
int SysRetransDelCurrentMsg(uint8_t id);
int SysRetransMsgLoad(uint8_t id, char *data, uint16_t dlen);
int SysRetransMsgSave(uint8_t id, const char* msg);
void SysRecordAverageData(float array[], uint8_t num);

//OTA升级
void SysOTAInfoSave(YMOTAInfo_t *ota);
void SysOTADataWrite(uint32_t offset, uint8_t *data, uint32_t size);
void SysOTADataRead(uint32_t offset, uint8_t *data, uint32_t size);
void SysOTASectionsErase(uint32_t size);

uint32_t SysGetReportInterval(void);
void SysSetReportInterval(uint32_t interval);
const char *SysPrintDateTime(void);
struct YMDateTime_st *SysGetDateTime(void);
void SysSetDateTime(struct YMDateTime_st *dateTime);
void SysSetDateTimeFromRtc(void);
bool SysDatePastTime(struct YMDateTime_st lastDate, uint16_t passSecond);
bool SysUtcTimePast(uint32_t oldTime, uint16_t past);
uint32_t SysUtcTime(void);

void SysSetLocation(float latitude, float longitude);
SysLocation_t *SysGetLocation(void);

void SysStatusLedSet(uint8_t blink);
void SysSetSensorConfig(void);
uint16_t SysGetSensorConfig(void);
void SysSetDeviceIDPwd(const char *idPwd);

void SysSetDevType(const char *type);
const char *SysGetDevType(void);
const char *SysGetDevicePwd(void);
const char *SysGetDeviceID(void);
const uint8_t *SysGetVersion(void);

void SysInterruptSet(bool enable);
void SysReboot(void);
void SysInfoPrint(void);
void SysInitialize(void);
void SysPoll(void);

#define WS_Select                 ((uint16_t)0x0001)  /*!<  selected */
#define WD_Select                 ((uint16_t)0x0002)  /*!<  selected */
#define TH_Select                 ((uint16_t)0x0004)  /*!<  selected */
#define CO_Select                 ((uint16_t)0x0008)  /*!<  selected */
#define NO2_Select                ((uint16_t)0x0010)  /*!<  selected */
#define SO2_Select                ((uint16_t)0x0020)  /*!<  selected */
#define O3_Select                 ((uint16_t)0x0040)  /*!<  selected */
#define NOISE_Select              ((uint16_t)0x0080)  /*!<  selected */
#define VOC_Select                ((uint16_t)0x0100)  /*!<  selected */
#define PM_Select                 ((uint16_t)0x0200)  /*!<  selected */

#define IS_SENSOR_SELECT(config, xx_Select)     ((config) & (xx_Select))



#endif

