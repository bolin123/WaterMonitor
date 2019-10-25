#include "Sys.h"
#include "SysTimer.h"
#include "SysCommand.h"
#include "APP.h"

#include "rtc.h"
#include "math.h"
#include "sd_api.h"
#include "ledpanel_protocol.h"
#include "PowerManager.h"
#include "Sensors.h"
#include "Manufacture.h"


#define SYS_SLEEP_MIN_INTERVAL 120

#pragma pack(1)
typedef struct
{
    unsigned char flag;
    unsigned char version[4];
    unsigned char md5[16];
    unsigned int  size;
    unsigned char reserve[3];
}SysOTAInfo_t;
#pragma pack()

static uint8_t g_version[4] = {1, 1, 7, 0};
static char g_devType[10];
static struct YMDateTime_st g_sysDateTime;
static SysLocation_t g_location;
static volatile bool g_rtcTimeup = false;
static uint16_t g_senserConfig = 0;

void *SysMalloc(uint16_t size)
{
    void *ptr = malloc(size);

    if(ptr)
    {
        return ptr;
    }
    else
    {
        SysLog("Malloc failed!!!");
        SysReboot();
    }
    return NULL;
}

void SysRetransInit(void)
{
    SysLog("");
    HalFileUpdateInfo();
}

int SysRetransMsgSave(uint8_t id, const char* msg)
{
    return HalFileInsertMessage(id, msg);
}

int SysRetransMsgLoad(uint8_t id, char *data, uint16_t dlen)
{
    return HalFileGetNextMessage(id, data, dlen);
}

int SysRetransDelCurrentMsg(uint8_t id)
{
    return HalFileDelLastMessage(id);
}

void SysRecordAverageData(float array[], uint8_t num)
{
    sd_record(AVERAGE_DATA_FILE_PATH, array, num);
}

void SysOTAInfoSave(YMOTAInfo_t *ota)
{
    SysOTAInfo_t otaInfo;

    if(ota != NULL)
    {
        otaInfo.flag = true;
        otaInfo.size = ota->size;
        memcpy(otaInfo.md5, ota->md5, sizeof(otaInfo.md5));
        memcpy(otaInfo.version, ota->version, sizeof(otaInfo.version));
        HalFlashErase(HAL_DEVICE_OTA_INFO_ADDR);
        HalFlashWrite(HAL_DEVICE_OTA_INFO_ADDR, (void *)&otaInfo, sizeof(SysOTAInfo_t));
    }
}

void SysOTASectionsErase(uint32_t size)
{
    int i;
    int count = (size + HAL_FLASH_PAGE_SIZE - 1) / HAL_FLASH_PAGE_SIZE;

    if(size < HAL_FLASH_OTA_SIZE)
    {
        for(i = 0; i < count; i++)
        {
            HalFlashErase(HAL_OTA_FLASH_ADDR + i * HAL_FLASH_PAGE_SIZE);
        }
    }
}

void SysOTADataWrite(uint32_t offset, uint8_t *data, uint32_t size)
{
    HalFlashWrite(HAL_OTA_FLASH_ADDR + offset, data, size);
}

void SysOTADataRead(uint32_t offset, uint8_t *data, uint32_t size)
{
    HalFlashRead(HAL_OTA_FLASH_ADDR + offset, data, size);
}

void SysSetLocation(float latitude, float longitude)
{
    g_location.fixed = true;
    g_location.latitude = latitude;
    g_location.longitude = longitude;
}

SysLocation_t *SysGetLocation(void)
{
    if(g_location.fixed)
    {
        return &g_location;
    }
    else
    {
        return NULL;
    }
}

void SysSetReportInterval(uint32_t interval)
{
    SysLog("interval = %d", interval);
    HalFlashWriteInPage(HAL_ARGS_INTERVAL_ADDR, &interval, sizeof(uint32_t));
}

uint32_t SysGetReportInterval(void)
{
    uint32_t interval;
    HalFlashRead(HAL_ARGS_INTERVAL_ADDR, &interval, sizeof(uint32_t));
    if((interval == 0xffffffff)||(interval == 0))
    {
        interval = 60; //default report interval, 60s
    }
    interval = 60 * ceil((float)interval / 60.0);
    return interval;
}

void SysSetDeviceIDPwd(const char *idPwd)
{
    SysLog("ID:%s", idPwd);
    HalFlashErase(HAL_DEVICE_ID_FLASH_ADDR);
    HalFlashWrite(HAL_DEVICE_ID_FLASH_ADDR, idPwd, SYS_DEVICE_ID_LEN + SYS_PASSWORD_LEN);
}

const char *SysGetDevicePwd(void)
{
    static char password[SYS_PASSWORD_LEN + 1] = "";

    if(password[0] == 0)
    {
        HalFlashRead(HAL_DEVICE_ID_FLASH_ADDR + SYS_DEVICE_ID_LEN, password, sizeof(password)-1);
    }
    return password;
}

const char *SysGetDeviceID(void)
{
    static char dvid[SYS_DEVICE_ID_LEN + 1] = "";

    if(dvid[0] == 0)
    {
        HalFlashRead(HAL_DEVICE_ID_FLASH_ADDR, dvid, sizeof(dvid)-1);
    }
    return dvid;
}

void SysStatusLedSet(uint8_t blink)
{
    HalCommonStatusLedSet(blink);
}

void SysSetDevType(const char *type)
{
    strcpy(g_devType, type);
}

const char *SysGetDevType(void)
{
    return g_devType;
}

void SysSetSensorConfig()
{
    if(strstr(SysGetDevType(),"YT-001") || strstr(SysGetDevType(),"YT-021"))
        g_senserConfig = WS_Select|WD_Select|TH_Select|CO_Select|NO2_Select|SO2_Select|O3_Select|PM_Select;
    else if(strstr(SysGetDevType(),"YT-003"))
        g_senserConfig = WS_Select|WD_Select|TH_Select|PM_Select;
    else if(strstr(SysGetDevType(),"YT-004") || strstr(SysGetDevType(),"YT-014") || strstr(SysGetDevType(),"YT-024"))
        g_senserConfig = WS_Select|WD_Select|TH_Select|PM_Select|NOISE_Select;
    else if(strstr(SysGetDevType(),"YT-005") || strstr(SysGetDevType(),"YT-025"))
        g_senserConfig = WS_Select|WD_Select|TH_Select|CO_Select|NO2_Select|SO2_Select|O3_Select|VOC_Select|PM_Select;
    else
        g_senserConfig = WS_Select|WD_Select|TH_Select|CO_Select|NO2_Select|SO2_Select|O3_Select|NOISE_Select|VOC_Select|PM_Select;
}

uint16_t SysGetSensorConfig()
{
    return g_senserConfig;
}

const uint8_t *SysGetVersion(void)
{
    return g_version;
}

static bool isLeapYear(uint16_t year)
{
    if(year && year % 4 == 0)
    {
        if(year % 100 == 0)
        {
            if(year % 400 != 0)
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

static void dateMonthUpdate(uint8_t day)
{
    uint8_t monthDays;
    bool isLeap = isLeapYear(g_sysDateTime.year);

    switch(g_sysDateTime.month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        monthDays = 31;
        break;

    case 2:
        monthDays = isLeap ? 29 : 28;
        break;
    default:
        monthDays = 30;
    }

    if(day > monthDays)
    {
        g_sysDateTime.month++;
        if(g_sysDateTime.month > 12)
        {
            g_sysDateTime.year++;
            g_sysDateTime.month = 1;
        }
        g_sysDateTime.day = 1;
    }
}

static void dateTimeUpdate(void)
{
    static SysTime_t lastTime;

    if(SysTime() - lastTime >= 1000)
    {
        g_sysDateTime.sec++;
        if(g_sysDateTime.sec > 59)
        {
            g_sysDateTime.sec = 0;
            g_sysDateTime.min++;
            if(g_sysDateTime.min > 59)
            {
                g_sysDateTime.min = 0;
                g_sysDateTime.hour++;
                if(g_sysDateTime.hour > 23)
                {
                    g_sysDateTime.hour = 0;
                    g_sysDateTime.day++;
                    if(g_sysDateTime.day > 28)
                    {
                        dateMonthUpdate(g_sysDateTime.day);
                    }
                }
            }
        }
        lastTime = SysTime();
    }
    g_sysDateTime.msec = SysTime() - lastTime;
}

void SysSetDateTime(struct YMDateTime_st *dateTime)
{
    if(dateTime)
    {
        g_sysDateTime = *dateTime;
    }
}
void SysSetDateTimeFromRtc(void)
{
    struct YMDateTime_st sysDateTime = rtc_get();
    //SysLog("[year:%d, month:%d, day:%d]", sysDataTime.year, sysDataTime.month, sysDataTime.day);
    if(sysDateTime.year >= 2017)
        SysSetDateTime(&sysDateTime);
}

struct YMDateTime_st *SysGetDateTime(void)
{
    dateTimeUpdate();
    return &g_sysDateTime;
}

bool SysDatePastTime(struct YMDateTime_st lastDate, uint16_t passSecond)//延时需要在1小时以内
{
    struct YMDateTime_st *currentDate = SysGetDateTime();

    if((currentDate->hour == 0 && lastDate.hour != 0)||(currentDate->hour > lastDate.hour))
    {
        if(((60 + currentDate->min - lastDate.min) * 60 + currentDate->sec - lastDate.sec) >= passSecond)
        {
            return true;
        }
    }
    else if(currentDate->hour == lastDate.hour)
    {
        if(((currentDate->min - lastDate.min) * 60 + currentDate->sec - lastDate.sec) >= passSecond)
        {
            return true;
        }
    }
    else
    {

    }

    return false;
}

bool SysUtcTimePast(uint32_t oldTime, uint16_t past)
{
    if(rtc_init_success())
    {
        if(rtc_utc_value() - oldTime > past)
        {
            return true;
        }
    }
    else
    {
        uint32_t ms = past * 1000;
        if(SysTime() - oldTime > ms)
        {
            return true;
        }
    }
    
    return false;
}

uint32_t SysUtcTime(void)
{
    return rtc_utc_value();
}

static char g_timeBuff[18];
const char *SysPrintDateTime(void)
{
    struct YMDateTime_st *time = &g_sysDateTime;

    sprintf(g_timeBuff, "[%02d:%02d:%02d.%03d]", time->hour, time->min, time->sec, time->msec);
    g_timeBuff[14] = '\0';
    return g_timeBuff;
}

void SysSetSleepMode(uint8_t mode)
{
    SysLog("mode = %d", mode);
    HalFlashWriteInPage(HAL_ARGS_SLEEP_MODE_ADDR, &mode, sizeof(uint8_t));
}

SysSleepMode_t SysGetSleepMode(void)
{
    uint8_t mode = 0xff;
    HalFlashRead(HAL_ARGS_SLEEP_MODE_ADDR, &mode, sizeof(uint8_t));
    if((mode == 0xff) || !rtc_init_success())
    {
        return SYS_SLEEP_MODE_NONE;
    }
    return (SysSleepMode_t)mode;
}

void SysSleepResume(void)
{
    HalCommonResume();
    //sysdatetime upgrade
}

bool SysIsRtcTimeup(void)
{
    return g_rtcTimeup;
}

void SysRtcTimeup(bool set)
{
    g_rtcTimeup = set;
}

bool SysNeedSleep(void)
{
    if(SysGetSleepMode() != SYS_SLEEP_MODE_NONE \
        && SysGetReportInterval() > SYS_SLEEP_MIN_INTERVAL \
        && rtc_init_success())
    {
        return true;
    }

    return false;
}

HalBootMode_t SysBootMode(void)
{
    return HalCommonBootMode();
}

static void sysSleepInit(void)
{
    if(SysNeedSleep())
    {
        rtc_alarm_set(rtc_get_counter() + SysGetReportInterval() - 1);
        PMStart();
    }
    else
    {
        PMStop();
    }
}


static uint32_t sysFreeRamSize(void)
{
#if 1
    uint16_t i = 0, j = 0;
    uint16_t msize = 2048;
    uint32_t count = 0;
    void *ptr[2][10] = {0};

    for(i = 0; i < 2; i++)
    {
        for(j = 0; j < 10; j++)
        {
            ptr[i][j] = malloc(msize);
            if(ptr[i][j] == NULL)
            {
                msize = 256;
                break;
            }
        }
    }

    for(i = 0; i < 2; i++)
    {
        for(j = 0; j < 10; j++)
        {
            if(ptr[i][j])
            {
                count += (i == 0 ? 2048 : 256);
                free(ptr[i][j]);
            }
        }
    }

    return count;
#endif
}

void SysReboot(void)
{
    SysLog("");
    HalCommonReboot();
}

void SysSleep(void)
{
#if 1
    SysSleepMode_t sleepMode = SysGetSleepMode();

    SysLog("");
    if(sleepMode == SYS_SLEEP_MODE_DEEP)
    {
        SysLog("deep");
        HalCommonStandBy();
    }
    else if(sleepMode == SYS_SLEEP_MODE_LIGHT)
    {
        SysLog("light");
        HalCommonSleep();
    }
    else
    {
    }
#endif
}

void SysInterruptSet(bool enable)
{
    static bool irqEnable = true;

    if(irqEnable != enable)
    {
        HalInterruptSet(enable);
    }

    irqEnable = enable;
}

void SysInfoPrint(void)
{
    SysPrintf("=========================================\n");
    SysPrintf("Device Type:%s\n", SysGetDevType());
    SysPrintf("Device ID:%s\n", SysGetDeviceID());
    SysPrintf("Version:%d.%d.%d.%d\n", g_version[0], g_version[1], g_version[2], g_version[3]);
    SysPrintf("Free mem:%d\n", sysFreeRamSize());
    SysPrintf("Compile Date:%s %s\n", __DATE__, __TIME__);
    SysPrintf("=========================================\n");
}

void SysInitialize(void)
{
    HalCommonInitialize();
    rtc_init();
    PMInitialize();
    SysTimerInitialize();
    SysCmdInit();
    SensorInitialize();
    sd_detectPin_init();
    APPInitialize();
    led_panel_init();
    sysSleepInit();
    SysInfoPrint();
    HalWatchdogInitialize();
    MAFInit();
}

void SysPoll(void)
{
    HalCommonPoll();
    SysTimerPoll();
    SysCmdPoll();
    dateTimeUpdate();
    APPPoll();
    led_panel_poll();
    PMPoll();
    SensorPoll();
}

