#include "SysCommand.h"

#define SYS_CMD_BUFF_MAX_LEN 128

/* 系统指令解析
* 1、设置设备ID、PIN码:
* {"cmd":"setid","value":"ID+PIN"}
* 2、读取设备信息
* {"cmd":"getInfo"}
* 3、设置传感器参数(工作极辅助极偏值电压)
* {"cmd":"getCalArgs","target":"NO2","aeoffset":"0"}
*/
static uint8_t g_cmdbuff[SYS_CMD_BUFF_MAX_LEN];
static uint8_t g_buffCount = 0;
static volatile bool g_gotCommand = false;

#if defined(HAL_BOARD_NAME_LITE)
static void setCalArgsHandler(char* target, char* type, char* value)
{
    char* sensorName[4]={"CO", "NO2", "SO2", "O3"};
    int i;
    for(i=0; i<4; i++)
    {
        if(strcmp(sensorName[i], target) == 0)
        {
            break;
        }
    }
    if(strcmp(type, "weoffset") == 0)
    {
        int midvalue1 = atoi(value);
        float midvalue2 = (float)midvalue1; 
        //SysPrintf("midvalue2=%.2f\n", midvalue2);
        HalFlashWriteInPage(HAL_ARGS_CO_WEOFFSET_ADDR + i*4, &midvalue2, sizeof(float));
    }
    else if(strcmp(type, "aeoffset") == 0)
    {
        int midvalue1 = atoi(value); 
        float midvalue2 = (float)midvalue1; 
        //SysPrintf("midvalue2=%.2f\n", midvalue2);
        HalFlashWriteInPage(HAL_ARGS_CO_AEOFFSET_ADDR + i*4, &midvalue2, sizeof(float));
    }       
}

static float getCalArgsHandler(char* target, char* type)
{
    char* sensorName[4]={"CO", "NO2", "SO2", "O3"};
    int i;
    for(i=0; i<4; i++)
    {
        if(strcmp(sensorName[i], target) == 0)
        {
            break;
        }
    }
    if(strcmp(type, "weoffset") == 0)
    {
        float value = 0.0;
        HalFlashRead(HAL_ARGS_CO_WEOFFSET_ADDR + i*4, &value, sizeof(float));
        return value;
    }
    else if(strcmp(type, "aeoffset") == 0)
    {
        float value  = 0.0;
        HalFlashRead(HAL_ARGS_CO_AEOFFSET_ADDR + i*4, &value, sizeof(float));
        return value;
    }      
    return 0.0;
}
static void setHGOffsetHandler()
{
    #include "ads1115.h"
    float ch[8];
    int8_t res1=0, res2=0;
    ch[7] = (float)ads1115_get_input_x(0x90, 0, &res1);//o3 ae
    ch[3] = (float)ads1115_get_input_x(0x90, 1, &res1);//o3 we
    ch[5] = (float)ads1115_get_input_x(0x90, 2, &res1);//no2 ae
    ch[1] = (float)ads1115_get_input_x(0x90, 3, &res1);//no2 we
    ch[6] = (float)ads1115_get_input_x(0x92, 0, &res2);//so2 ae
    ch[2] = (float)ads1115_get_input_x(0x92, 1, &res2);//so2 we
    ch[4] = (float)ads1115_get_input_x(0x92, 2, &res2);//co ae
    ch[0] = (float)ads1115_get_input_x(0x92, 3, &res2);//co we   
    if(res1+res2 < 0)
    {
        SysLog("i2c error, check the wiring");
        return;
    }
    HalFlashWriteInPage(HAL_ARGS_CO_WEOFFSET_ADDR, ch, sizeof(ch));
    SysLog("ad1: %.0f %.0f %.0f %.0f", ch[7], ch[3], ch[5], ch[1]);
    SysLog("ad2: %.0f %.0f %.0f %.0f", ch[6], ch[2], ch[4], ch[0]);
}
#endif

static char *getCmdValueString(const char *contents, const char *keyName)
{
    char *keyPos = strstr(contents, keyName);
    char *valStart, *valStop;
    char *value;

    if(keyPos)
    {
        valStart = strchr(keyPos, ':');
        if(valStart)
        {
            valStart = strchr(valStart, '"');
            if(valStart)
            {
                valStart += 1;
                valStop = strchr(valStart, '"');
                if(valStop)
                {
                    value = SysMalloc(valStop - valStart + 1);
                    if(value)
                    {
                        memcpy(value, valStart, valStop - valStart);
                        value[valStop - valStart] = '\0';
                        return value;
                    }
                }
            }
        }
    }
    return NULL;
}

static void cmdParse(const char *msg)
{
    char *opt = getCmdValueString(msg, "cmd");

    SysLog("%s", msg);
    
    if(opt)
    {
        if(strcmp(opt, "setid") == 0)
        {
            char *id = getCmdValueString(msg, "value");

            if(id)
            {
                SysSetDeviceIDPwd(id);
                free(id);
            }
        }
        else if(strcmp(opt, "getInfo") == 0)
        {
            SysInfoPrint();
        }
        #if defined(HAL_BOARD_NAME_LITE)
        else if(strcmp(opt, "setCalArgs") == 0)
        {
            char *id = getCmdValueString(msg, "target");
            if(id)
            {
                char *cid = getCmdValueString(msg, "weoffset");
                if(cid)
                {
                    setCalArgsHandler(id, "weoffset", cid);
                    free(cid);
                }
                cid = getCmdValueString(msg, "aeoffset");
                if(cid)
                {
                    setCalArgsHandler(id, "aeoffset", cid);
                    free(cid);
                }
                free(id);
            }
            
        }
        else if(strcmp(opt, "getCalArgs") == 0)
        {
            char *id = getCmdValueString(msg, "target");
            if(id)
            {
                char *cid = getCmdValueString(msg, "weoffset");
                if(cid)
                {
                    SysPrintf("cmdResp: {weoffset = %.2f}\n", getCalArgsHandler(id, "weoffset"));
                    free(cid);
                }
                cid = getCmdValueString(msg, "aeoffset");
                if(cid)
                {
                    SysPrintf("cmdResp: {aeoffset = %.2f}\n", getCalArgsHandler(id, "aeoffset"));
                    free(cid);
                }
                free(id);
            }
            
        }
        else if(strcmp(opt, "setHGOffset") == 0)
        {
            setHGOffsetHandler();            
        }
        #endif
        else
        {
        }

        free(opt);
    } 

}

void SysCmdByteRecv(uint8_t *data, uint16_t len)
{
    static SysTime_t lastRecvTime;

    if(g_gotCommand)
    {
        return;
    }

    for(uint16_t i = 0; i < len; i++)
    {
        if(SysTimeHasPast(lastRecvTime, 200))
        {
            g_buffCount = 0;
        }
        lastRecvTime = SysTime();

        g_cmdbuff[g_buffCount++] = data[i];

        if(g_buffCount == 0)
        {
            if(data[i] != '{')
            {
                g_buffCount = 0;
            }
        }
        else
        {
            if(data[i] == '}')
            {
                g_cmdbuff[g_buffCount] = '\0';
                g_gotCommand = true;
                //cmdParse((const char *)cmdbuff);
            }
        }

        if(g_buffCount >= SYS_CMD_BUFF_MAX_LEN)
        {
            g_buffCount = 0;
        }
    }
    
}

void SysCmdInit(void)
{
}

void SysCmdPoll(void)
{
    if(g_gotCommand)
    {
        cmdParse((const char *)g_cmdbuff);
        g_buffCount = 0;
        g_gotCommand = false;
    }
}

