#include "YMPropertyManager.h"
#include "YMCtype.h"
#include "YMList.h"
#include "Yumair.h"

typedef struct YMProperties_st
{
    char *name;
    float value;
    char *flag;
    YmUint8_t flagValue;
    YMLIST_ENTRY(struct YMProperties_st);
}YMProperties_t;

static YMProperties_t g_properties;

_funcTag const char * YMPMProperties2Text(void)
{
    YmUint16_t memSize = 0;
    YmUint16_t pos = 0;
    char *text = YmNULL;
    char *ptr;
    char valStr[32] = "";
    YMProperties_t *node;

    text = (char *)YmMalloc(25);
    if (text)
    {
        YMDateTime_t *date = YM_GET_SYS_DATE_TIME();
        sprintf(text, "DataTime=%04d%02d%02d%02d%02d%02d;", date->year, date->month, date->day, \
            date->hour, date->min, date->sec);
        memSize = 25;
        pos = strlen(text);
    }
    else
    {
        goto mem_release;
    }

    YMListForeach(&g_properties, node)
    {
        sprintf(valStr, "%.04f", node->value);
        if (node->flag != YmNULL)
        {
            memSize += strlen(valStr) + strlen(node->name) + strlen(node->flag) + 1 + 4;
            ptr = realloc(text, memSize);
            if (ptr)
            {
                text = ptr;
                pos += sprintf(text + pos, "%s=%s,%s=%1d;", node->name, valStr, node->flag, node->flagValue);
            }
            else
            {
                goto mem_release;
            }
        }
        else
        {
            memSize += strlen(valStr) + strlen(node->name) + 2;
            ptr = realloc(text, memSize);
            if (ptr)
            {
                text = ptr;
                pos += sprintf(text + pos, "%s=%s;", node->name, valStr);
            }
            else
            {
                goto mem_release;
            }
        }
    }

    return text;
mem_release:

    if (text)
    {
        free(text);
    }
    return YmNULL;
}

_funcTag int YMPMRegister(const char * name, const char * flagName)
{
    YMProperties_t *property = YmNULL;

    if (name != YmNULL)
    {
        property = (YMProperties_t *)YmMalloc(sizeof(YMProperties_t));
        if (property != YmNULL)
        {
            property->name = (char *)YmMalloc(strlen(name) + 1);
            if (property->name)
            {
                strcpy(property->name, name);
                property->value = 0.0;
            }

            if (flagName)
            {
                property->flag = (char *)YmMalloc(strlen(flagName) + 1);
                if (property->flag)
                {
                    strcpy(property->flag, flagName);
                    property->flagValue = 0;
                }
            }
            else
            {
                property->flag = YmNULL;
            }
            YMListAdd(&g_properties, property);
            return 0;
        }
    }
    return -1;
}

_funcTag int YMPMSetValue(const char * name, float value, YmUint8_t flag)
{
    YMProperties_t *node;

    YMListForeach(&g_properties, node)
    {
        if (strcmp(node->name, name) == 0)
        {
            node->value = value;

            if (node->flag)
            {
                node->flagValue = flag;
            }
            return 0;
        }
    }
    return -1;
}

_funcTag void YMPMInitialize(void)
{
    YMListInit(&g_properties);
}

_funcTag void YMPMPoll(void)
{
}


