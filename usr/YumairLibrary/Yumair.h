#ifndef YUMAIR_H
#define YUMAIR_H

/*支持的传感器类型(用户自定义)
*传感器类型和名称顺序要一一对应
*/
typedef enum
{
    YM_SENSOR_TYPE_TP=0, //水温
    YM_SENSOR_TYPE_EC,   //电导率
    YM_SENSOR_TYPE_PH,   //pH值
    YM_SENSOR_TYPE_O2,   //溶解氧
    YM_SENSOR_TYPE_TB,   //浊度
    YM_SENSOR_TYPE_COD,  //COD
    YM_SENSOR_TYPE_TOC,  //TOC
    YM_SENSOR_TYPE_DEP,  //水位
    YM_SENSOR_TYPE_FLOW, //流量
    YM_SENSOR_TYPE_SS,   //悬浮物
    YM_SENSOR_TYPE_NH4,  //氨氮
    YM_SENSOR_TYPE_CHRM, //色度
    YM_SENSOR_TYPE_BOD,  //BOD
    YM_SENSOR_TYPE_COUNT,
}YMSensorType_t;
static const char *YMSensorName[YM_SENSOR_TYPE_COUNT] = {"TP", "EC", "PH", "O2", "TB", "COD", "TOC", "DEP", "FLOW", "SS", "NH4", "CHRM", "BOD"};

/*系统事件*/
typedef enum
{
    YM_EVENT_REBOOT = 0,          //重启
    YM_EVENT_GET_SENSOR_ARGS,     //读取传感器值
    YM_EVENT_SET_SENSOR_ARGS,     //设置传感器值
    YM_EVENT_SET_POST_INTERVAL,   //设置上报时间间隔
    YM_EVENT_OTA_STATUS,          //OTA状态
    YM_EVENT_TIMMING,             //校时信息
    YM_EVENT_GET_LOCATION,        //读取位置信息
    YM_EVENT_SERVER_STATUS,       //服务器连接状态改变
    YM_EVENT_SET_SLEEPMODE,       //设置休眠模式
    YM_EVENT_LINK_ABNORMAL,       //连接异常
}YMEvent_t;

typedef enum
{
    YM_SERVER_LINK_DISCONNECTED = 0, //未连接
    YM_SERVER_LINK_CONNECTED,        //连接成功
    YM_SERVER_LINK_LOGIN,            //登录成功
    //YM_SERVER_LINK_ABNORMAL,         //连接异常
}YMServerLink_t;

/*服务器ID*/
typedef enum
{
    YM_SERVER_ID_MASTER = 0,
    YM_SERVER_ID_SLAVE,
}YMServerId_t;

/*应答消息类型*/
typedef enum
{
    YM_REPLY_TYPE_LOCATION,       //位置信息
    YM_REPLY_TYPE_SENSOR_PARAM,   //传感器参数
    YM_REPLY_TYPE_RESULT,         //执行结果
}YMReplyType_t;

/*操作结果*/
typedef enum
{
    YM_PROCESS_RESULT_SUCCESS = 1,   //执行成功
    YM_PROCESS_RESULT_FAILED  = 2,   //执行失败，未知原因
    YM_PROCESS_RESULT_NO_DATA = 100, //没有数据
}YMProcessResult_t;

/*OTA升级状态*/
typedef enum
{
    YM_OTA_STATUS_START,          //开始升级
    YM_OTA_STATUS_PROGRESS,       //升级进度
    YM_OTA_STATUS_RESULT_SUCCESS, //升级成功
    YM_OTA_STATUS_RESULT_FAILED,  //升级失败
}YMOTAStatus_t;

/*OTA升级结果信息*/
typedef struct 
{
    unsigned char version[4];
    unsigned char md5[16];
    unsigned int  size;
}YMOTAInfo_t;

/*OTA升级进度*/
typedef struct
{
    YMOTAStatus_t status;
    unsigned char progress;
    YMOTAInfo_t *info;
}YMOTAData_t;

/*服务器连接状态*/
typedef struct
{
    YMServerId_t sid;
    YMServerLink_t status;
}YMServerStatus_t;


/*系统日期时间结构体*/
typedef struct YMDateTime_st
{
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char min;
    unsigned char sec;
    unsigned short msec;
}YMDateTime_t;

/*位置信息*/
typedef struct
{
    float latitude;  //纬度
    float longitude; //精度
}YMLocationInfo_t;

/*传感器参数*/
typedef struct
{
    unsigned char method;
    unsigned char valnum;
    float *value;
    int target;
}YMSensorParam_t;

/*事件回调函数
* @event:事件类型， @args:参数
* @ackid：应答ID，有些事件需要应答，若ackid＞0，则需要调用YMReply函数进行应答
*/
typedef void(*YMEventCallback_t)(YMEvent_t event, void *args, unsigned int ackid);

/* 初始化函数
* @eventHandle：事件回调函数
*/
void YMInitialize(YMEventCallback_t eventHandle);

/*主循环函数
* 事件及业务处理，循环调用
*/
void YMPoll(void);

/*开始执行函数
*/
void YMStart(void);

/*停止执行函数
*/
void YMStop(void);

/*1毫秒计时函数
* 1毫秒调用一次该函数
*/
void YM1msPast(void);

/*设置固件版本
* @version:版本号(4位，eg.version = {1, 0, 0, 1})
*/
void YMSetFirmVersion(const unsigned char *version);

/*设置设备型号
* @model:设备型号
*/
void YMSetDeviceModel(const char *model);

/*设置设备ID和PIN码
* @id:设备ID, @pwd:设备PIN码
*/
void YMSetDevIDAndPwd(const char *id, const char *pwd);

/*设置上传时间间隔
* @seconds:上传数据的时间间隔（秒）
*/
void YMSetReportInterval(unsigned short seconds);


/*设置休眠模式
* @mode:休眠模式
*/
void YMSetSleepMode(unsigned char mode);

/*更新日期时间
* @time:日期时间
*/
void YMSetDateTime(YMDateTime_t *time);

/*注册数据属性
* @name:属性名称(如，PM2.5-Rtd)， @flagName：属性标记名称(如，PM2.5-Flag)
*/
int YMPropertyRegister(const char *name, const char *flagName);

/*设置数据属性的值
* @name:属性名称， @value:属性值， @flag：标记值（0：校准数据、1：气象参数、2：异常数据、3：正常数据）
*/
int YMPropertySet(const char *name, float value, unsigned char flag);

/*上传全部属性值
* 根据上传时间间隔周期调用该函数，每次调用都会上传一次属性值
*/
int YMPostAllProperties(void);

/*请求校时
@返回值: < 0 = 失败, 0 = 成功
*/
int YMRequestTiming(void);

/*上传位置信息
* @latitude：纬度， @longitude：经度
*/
void YMLocationInfoReport(float latitude, float longitude);

/*设置/清除异常码
* @faultNum：异常码， @set:YmTrue=设置，YmFalse=清除
* 若异常码有变化则上报该异常
*/
void YMFaultsNumSet(unsigned char faultNum, unsigned char set);

/*设备应答(回复服务器请求消息)
* @replyID:回复ID（同事件回调函数中的ackid）, @type:应答类型， @args:应答参数
*/
void YMReply(unsigned int replyID, YMReplyType_t type, YMProcessResult_t result, void *args);


#endif // !YUMAIR_H

