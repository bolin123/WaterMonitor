#ifndef YUMAIR_H
#define YUMAIR_H

/*֧�ֵĴ���������(�û��Զ���)
*���������ͺ�����˳��Ҫһһ��Ӧ
*/
typedef enum
{
    YM_SENSOR_TYPE_TP=0, //ˮ��
    YM_SENSOR_TYPE_EC,   //�絼��
    YM_SENSOR_TYPE_PH,   //pHֵ
    YM_SENSOR_TYPE_O2,   //�ܽ���
    YM_SENSOR_TYPE_TB,   //�Ƕ�
    YM_SENSOR_TYPE_COD,  //COD
    YM_SENSOR_TYPE_TOC,  //TOC
    YM_SENSOR_TYPE_DEP,  //ˮλ
    YM_SENSOR_TYPE_FLOW, //����
    YM_SENSOR_TYPE_SS,   //������
    YM_SENSOR_TYPE_NH4,  //����
    YM_SENSOR_TYPE_CHRM, //ɫ��
    YM_SENSOR_TYPE_BOD,  //BOD
    YM_SENSOR_TYPE_COUNT,
}YMSensorType_t;
static const char *YMSensorName[YM_SENSOR_TYPE_COUNT] = {"TP", "EC", "PH", "O2", "TB", "COD", "TOC", "DEP", "FLOW", "SS", "NH4", "CHRM", "BOD"};

/*ϵͳ�¼�*/
typedef enum
{
    YM_EVENT_REBOOT = 0,          //����
    YM_EVENT_GET_SENSOR_ARGS,     //��ȡ������ֵ
    YM_EVENT_SET_SENSOR_ARGS,     //���ô�����ֵ
    YM_EVENT_SET_POST_INTERVAL,   //�����ϱ�ʱ����
    YM_EVENT_OTA_STATUS,          //OTA״̬
    YM_EVENT_TIMMING,             //Уʱ��Ϣ
    YM_EVENT_GET_LOCATION,        //��ȡλ����Ϣ
    YM_EVENT_SERVER_STATUS,       //����������״̬�ı�
    YM_EVENT_SET_SLEEPMODE,       //��������ģʽ
    YM_EVENT_LINK_ABNORMAL,       //�����쳣
}YMEvent_t;

typedef enum
{
    YM_SERVER_LINK_DISCONNECTED = 0, //δ����
    YM_SERVER_LINK_CONNECTED,        //���ӳɹ�
    YM_SERVER_LINK_LOGIN,            //��¼�ɹ�
    //YM_SERVER_LINK_ABNORMAL,         //�����쳣
}YMServerLink_t;

/*������ID*/
typedef enum
{
    YM_SERVER_ID_MASTER = 0,
    YM_SERVER_ID_SLAVE,
}YMServerId_t;

/*Ӧ����Ϣ����*/
typedef enum
{
    YM_REPLY_TYPE_LOCATION,       //λ����Ϣ
    YM_REPLY_TYPE_SENSOR_PARAM,   //����������
    YM_REPLY_TYPE_RESULT,         //ִ�н��
}YMReplyType_t;

/*�������*/
typedef enum
{
    YM_PROCESS_RESULT_SUCCESS = 1,   //ִ�гɹ�
    YM_PROCESS_RESULT_FAILED  = 2,   //ִ��ʧ�ܣ�δ֪ԭ��
    YM_PROCESS_RESULT_NO_DATA = 100, //û������
}YMProcessResult_t;

/*OTA����״̬*/
typedef enum
{
    YM_OTA_STATUS_START,          //��ʼ����
    YM_OTA_STATUS_PROGRESS,       //��������
    YM_OTA_STATUS_RESULT_SUCCESS, //�����ɹ�
    YM_OTA_STATUS_RESULT_FAILED,  //����ʧ��
}YMOTAStatus_t;

/*OTA���������Ϣ*/
typedef struct 
{
    unsigned char version[4];
    unsigned char md5[16];
    unsigned int  size;
}YMOTAInfo_t;

/*OTA��������*/
typedef struct
{
    YMOTAStatus_t status;
    unsigned char progress;
    YMOTAInfo_t *info;
}YMOTAData_t;

/*����������״̬*/
typedef struct
{
    YMServerId_t sid;
    YMServerLink_t status;
}YMServerStatus_t;


/*ϵͳ����ʱ��ṹ��*/
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

/*λ����Ϣ*/
typedef struct
{
    float latitude;  //γ��
    float longitude; //����
}YMLocationInfo_t;

/*����������*/
typedef struct
{
    unsigned char method;
    unsigned char valnum;
    float *value;
    int target;
}YMSensorParam_t;

/*�¼��ص�����
* @event:�¼����ͣ� @args:����
* @ackid��Ӧ��ID����Щ�¼���ҪӦ����ackid��0������Ҫ����YMReply��������Ӧ��
*/
typedef void(*YMEventCallback_t)(YMEvent_t event, void *args, unsigned int ackid);

/* ��ʼ������
* @eventHandle���¼��ص�����
*/
void YMInitialize(YMEventCallback_t eventHandle);

/*��ѭ������
* �¼���ҵ����ѭ������
*/
void YMPoll(void);

/*��ʼִ�к���
*/
void YMStart(void);

/*ִֹͣ�к���
*/
void YMStop(void);

/*1�����ʱ����
* 1�������һ�θú���
*/
void YM1msPast(void);

/*���ù̼��汾
* @version:�汾��(4λ��eg.version = {1, 0, 0, 1})
*/
void YMSetFirmVersion(const unsigned char *version);

/*�����豸�ͺ�
* @model:�豸�ͺ�
*/
void YMSetDeviceModel(const char *model);

/*�����豸ID��PIN��
* @id:�豸ID, @pwd:�豸PIN��
*/
void YMSetDevIDAndPwd(const char *id, const char *pwd);

/*�����ϴ�ʱ����
* @seconds:�ϴ����ݵ�ʱ�������룩
*/
void YMSetReportInterval(unsigned short seconds);


/*��������ģʽ
* @mode:����ģʽ
*/
void YMSetSleepMode(unsigned char mode);

/*��������ʱ��
* @time:����ʱ��
*/
void YMSetDateTime(YMDateTime_t *time);

/*ע����������
* @name:��������(�磬PM2.5-Rtd)�� @flagName�����Ա������(�磬PM2.5-Flag)
*/
int YMPropertyRegister(const char *name, const char *flagName);

/*�����������Ե�ֵ
* @name:�������ƣ� @value:����ֵ�� @flag�����ֵ��0��У׼���ݡ�1�����������2���쳣���ݡ�3���������ݣ�
*/
int YMPropertySet(const char *name, float value, unsigned char flag);

/*�ϴ�ȫ������ֵ
* �����ϴ�ʱ�������ڵ��øú�����ÿ�ε��ö����ϴ�һ������ֵ
*/
int YMPostAllProperties(void);

/*����Уʱ
@����ֵ: < 0 = ʧ��, 0 = �ɹ�
*/
int YMRequestTiming(void);

/*�ϴ�λ����Ϣ
* @latitude��γ�ȣ� @longitude������
*/
void YMLocationInfoReport(float latitude, float longitude);

/*����/����쳣��
* @faultNum���쳣�룬 @set:YmTrue=���ã�YmFalse=���
* ���쳣���б仯���ϱ����쳣
*/
void YMFaultsNumSet(unsigned char faultNum, unsigned char set);

/*�豸Ӧ��(�ظ�������������Ϣ)
* @replyID:�ظ�ID��ͬ�¼��ص������е�ackid��, @type:Ӧ�����ͣ� @args:Ӧ�����
*/
void YMReply(unsigned int replyID, YMReplyType_t type, YMProcessResult_t result, void *args);


#endif // !YUMAIR_H

