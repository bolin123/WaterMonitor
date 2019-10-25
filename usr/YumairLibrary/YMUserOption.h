#ifndef YM_USER_OPTION_H
#define YM_USER_OPTION_H

#ifndef _funcTag
#define _funcTag 
#endif

/*主、从服务器地址信息*/
#define YM_MASTER_SERVER_URL  "server.yumair.cn"  //服务器域名
#define YM_MASTER_SERVER_PORT 10067              //端口号

#if YM_SLAVE_SERVER_ENABLE
#define YM_SLAVE_SERVER_URL "test.machtalk.net"  //服务器域名
#define YM_SLAVE_SERVER_PORT 10066               //端口号
#endif // YM_SLAVE_SERVER_ENABLE

/*系统日期时间结构体*/
#define YmTime_t YmUint32_t

struct YMDateTime_st;
struct YMDateTime_st *SysGetDateTime(void);
#define YM_GET_SYS_DATE_TIME SysGetDateTime

#ifndef YM_GET_SYS_DATE_TIME
#define YM_USE_YUMAIR_DATE_TIME 1
struct YMDateTime_st *YMGetSysDateTime(void); //获取系统时间
#define YM_GET_SYS_DATE_TIME YMGetSysDateTime
#endif // !YM_GET_SYS_DATE_TIME

/*断点续传*/
#if YM_RESUME_TRANSFER_ENABLE
void SysRetransInit(void);
int SysRetransDelCurrentMsg(YmUint8_t id);
int SysRetransMsgLoad(YmUint8_t id, char *data, YmUint16_t dlen);
int SysRetransMsgSave(YmUint8_t id, const char* msg);

#define YM_RETRANS_UPDATE_INFO()                     SysRetransInit()
#define YM_RETRANS_SAVE_LOST_DATA(id, message)       SysRetransMsgSave(id, message)
#define YM_RETRANS_LOAD_NEXT_DATA(id, buff, bufflen) SysRetransMsgLoad(id, buff, bufflen)
#define YM_RETRANS_DEL_CURRENT_DATA(id)              SysRetransDelCurrentMsg(id)
#endif // YM_RESUME_TRANSFER_ENABLE

/*OTA升级*/
#if YM_OTA_ENABLE
void SysOTASectionsErase(YmUint32_t size);
void SysOTADataWrite(YmUint32_t offset, YmUint8_t *data, YmUint32_t size);
void SysOTADataRead(YmUint32_t offset, YmUint8_t *data, YmUint32_t size);

#define YM_OTA_SECTIONS_ERASE(size)           SysOTASectionsErase(size)
#define YM_OTA_DATA_WRITE(offset, data, wlen) SysOTADataWrite(offset, data, wlen)
#define YM_OTA_DATA_READ(offset, data, rlen)  SysOTADataRead(offset, data, rlen)
#endif // YM_OTA_ENABLE


YmTime_t YMSysTime(void); //当前时间戳
#define YMSysTimeHasPast(lastTime, pastTime) (YMSysTime() - (lastTime) > (pastTime)) //时间比较函数

/*日志*/
#if YM_OPTION_ENABLE_LOG_PRINT
#define YMPrint printf //TODO: 定义系统打印函数
#define YMLog(...) YMPrint("%s[%d]:", __FUNCTION__, __LINE__);YMPrint(__VA_ARGS__);YMPrint("\n")
#else
#define YMPrint
#define YMLog(...)
#endif // YM_OPTION_ENABLE_LOG_PRINT

#endif // !YM_USER_OPTION_H

