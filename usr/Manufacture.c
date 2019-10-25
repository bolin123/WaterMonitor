#include "Manufacture.h"
#include "sys.h"
#include "rtc.h"
#include "sd_api.h"
#include "sensors.h"
#include "SysTimer.h"
#include "GPRS.h"

/*******************************************************************************
 * define and typedef
 ******************************************************************************/

typedef struct
{
    uint8_t precode[4];
    uint8_t len;
    uint8_t check;
    uint8_t type;
    uint8_t data[];
}SFrame_t;

typedef enum
{
    TEST_CMD_TEST=0,
    TEST_CMD_WRITE_INFO,
    TEST_CMD_READ_INFO,
    TEST_CMD_START_TEST,
    TEST_CMD_RESP_RESULT,
}TestCMD_t;

typedef enum
{
    TEST_RES_ERROR=0,
    TEST_RES_SUCCESS,
    TEST_RES_DATA,
}TestResult_t;

typedef struct
{
    TestType_t type;
    TestResult_t res;
    uint8_t data[];
}SFrameTestResp_t;


/*******************************************************************************
 * declaration func
 ******************************************************************************/
static void handleTest(void);


/*******************************************************************************
 * global variable
 ******************************************************************************/

static uint8_t g_frameBuf[256];
static uint8_t g_frameBufCount = 0;
static SysTime_t g_runTime = 0;
static bool g_beginTest = false;
    
/*static uint8_t calcChecksum(const uint8_t *data, uint8_t len)
{
    uint8_t check = 0;
    uint8_t i;
    for(i = 0; i < len; i++)
    {
        check += data[i];
    }
    return check;
}*/

static uint8_t crc8(uint8_t *ptr, uint8_t len)
{
	uint8_t crc;
	uint8_t i;
    crc = 0;
    while(len--)
    {
       crc ^= *ptr++;
       for(i = 8;i > 0;--i)
       {
           if(crc & 0x80)
           {
               crc = (crc << 1) ^ 0x131;
           }
           else
           {
               crc <<= 1;
           }
       }
    }
    return crc;
}


static void sendFrame(uint8_t type, const uint8_t *data, uint8_t len)
{
    SysLog("");

    uint8_t buf[100];
    SFrame_t *frame = (SFrame_t *)buf;
    frame->precode[0] = 0xaa;
    frame->precode[1] = 0xbb;
    frame->precode[2] = 0xcc;
    frame->precode[3] = 0xdd;
    frame->len = 2 + len;
    frame->type = type;
    memcpy(frame->data, data, len);
    frame->check = crc8(&frame->check + 1, frame->len - 1);


    uint8_t i;
    for(i = 0; i < frame->len + 5; i++)
    {
        SysPrintf("%02x ", buf[i]);
    }
    SysPrintf("\n");
    HalUartWrite(HAL_UART_PM_PORT, buf, frame->len + 5);
}

static void processUartByte(void)
{
    SFrame_t *frame = (SFrame_t *)g_frameBuf;
    switch(frame->type)
    {
        case TEST_CMD_TEST:
		{
            if(strstr((const char*)(frame->data), "TEST"))
            {
                g_beginTest = true;
                sendFrame(TEST_CMD_TEST, NULL, 0);
            }
            break;
		}
        case TEST_CMD_WRITE_INFO:
		{
            if(g_beginTest && (frame->len == 2 + SYS_DEVICE_ID_LEN + SYS_PASSWORD_LEN))
            {
                SysSetDeviceIDPwd((const char*)frame->data);
                sendFrame(TEST_CMD_WRITE_INFO, NULL, 0);
            }
            break;
		}
        case TEST_CMD_READ_INFO:
		{
            if(g_beginTest && strstr((const char*)(frame->data), "READID"))
            {
                char idPwd[SYS_DEVICE_ID_LEN + SYS_PASSWORD_LEN + 1] = "";
                const char* id = SysGetDeviceID();
                const char* pwd = SysGetDevicePwd();
                strcpy(idPwd, id);
                strcat(idPwd, pwd);
                SysPrintf("IdPwd: %s \n", idPwd);
                sendFrame(TEST_CMD_READ_INFO, (const unsigned char*)idPwd, strlen(idPwd));
            }
            break;
		}
        case TEST_CMD_START_TEST:
		{
            if(g_beginTest && strstr((const char*)(frame->data), "START"))
            {
                sendFrame(TEST_CMD_START_TEST, NULL, 0);
                handleTest();
            }
            break;
		}
        default:
            break;
    }
}

static void preprocessUartByte(uint8_t byte)
{
    static bool getPrecode = false;
    g_frameBuf[g_frameBufCount++] = byte;
    switch(g_frameBufCount)
    {
        case 1:
            if(byte != 0xaa)
            {
                g_frameBufCount = 0;
                getPrecode = false;
            }
            break;
        case 2:
        case 3:
        case 4:
            if((getPrecode == false) && (byte == 0xaa))
            {
                g_frameBuf[0] = 0xaa;
                g_frameBufCount = 1;
                break;
            }
            else if(g_frameBufCount == 4)
            {
                if((g_frameBuf[0] == 0xaa) && (g_frameBuf[1] == 0xbb) \
                                     && (g_frameBuf[2] == 0xcc) && (g_frameBuf[3] == 0xdd))
                {
                    getPrecode = true;
                }
                else
                {
                    g_frameBufCount = 0;
                }
            }
            break;
        case 5:
            if(byte > sizeof(g_frameBuf) - 5 || byte < 2)
            {
                g_frameBufCount = 0;
            }
            break;

        default:
            if(g_frameBufCount >= sizeof(SFrame_t)
               && g_frameBufCount >= g_frameBuf[4] + 5)
            {
                if(crc8(g_frameBuf + 6, g_frameBuf[4] - 1) == g_frameBuf[5])
                {
                    processUartByte();
                }
                g_frameBufCount = 0;
                memset(g_frameBuf, 0, sizeof(g_frameBuf));
                getPrecode = false;
            }
    }
}

void MAFDataPreprocess(const uint8_t *data, uint16_t len)
{
    if(!SysTimeHasPast(g_runTime, 60*1000) || g_beginTest)
    {
        uint16_t i;
        for(i = 0; i < len; i++)
        {
            preprocessUartByte(data[i]);
        }
    }
}

static void complete_fn(TestType_t testType, int8_t res, void* extra)
{
    static int8_t loopRes = 0;
    uint8_t buf[20];
    loopRes += res;
    SFrameTestResp_t *frame = (SFrameTestResp_t*)buf;
    //TestType_t testType = sensor2TestType(sensor);
    switch(testType)
    {
        case TEST_TYPE_WP:
        case TEST_TYPE_TH:
        case TEST_TYPE_I2C:
        case TEST_TYPE_RTC:
        case TEST_TYPE_SD:
        case TEST_TYPE_GPRS: 
        case TEST_TYPE_ID:
            frame->type = testType;
            frame->res = (TestResult_t)(res+1);
            sendFrame(TEST_CMD_RESP_RESULT, buf, sizeof(SFrameTestResp_t));
            break;
        case TEST_TYPE_ADC:
            frame->type = testType;
            frame->res = TEST_RES_DATA;
            memcpy(frame->data, extra, 5*sizeof(uint16_t));
            sendFrame(TEST_CMD_RESP_RESULT, buf, sizeof(SFrameTestResp_t)+5*sizeof(uint16_t));
            break;
        case TEST_TYPE_FULL:
            frame->type = testType;
            frame->res = ((loopRes < 0) ? TEST_RES_ERROR : TEST_RES_SUCCESS);
            loopRes = 0;
            sendFrame(TEST_CMD_RESP_RESULT, buf, sizeof(SFrameTestResp_t));
            break;
        default:
            break;
    }
}

static void gprsSelfTest(void)
{
    complete_fn(TEST_TYPE_GPRS, (true == IsGPRSComOk())?(0):(-1), NULL);
}

static void rtcSelfTest(void)
{
    complete_fn(TEST_TYPE_RTC, (true == rtc_init_success())?(0):(-1), NULL);
}

static void sdSelfTest(void)
{
    complete_fn(TEST_TYPE_SD, (0 == IsSDHalError())?(0):(-1), NULL);
}

static void idSelfTest(void)
{
    const char* id = SysGetDevicePwd();
    complete_fn(TEST_TYPE_ID, ((unsigned char)id[0] != 0xff)?(0):(-1), NULL);
}

void handleTest(void)
{
    SensorSelfTest(complete_fn);
    gprsSelfTest();
    rtcSelfTest();
    sdSelfTest();
	idSelfTest();
    complete_fn(TEST_TYPE_FULL, 0, NULL);
}

void MAFInit(void)
{
    g_runTime = SysTime();
}




