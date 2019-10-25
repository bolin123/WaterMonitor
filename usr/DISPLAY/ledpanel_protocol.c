#include "ledpanel_protocol.h"
#include "string.h"
#include "Sys.h"
#include "ledpanel_hardware.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "sensors.h"


#if HAL_LEDPANEL_USING

//小端模式
#pragma pack(1)
typedef struct
{//控制文件格式
	uint8_t fileType;
	uint8_t fileName[4];
	uint32_t fileLen;
	uint8_t controlName[8];
	uint16_t address;
	uint8_t baudrate;
	uint16_t screenWidth;
	uint16_t screenHeight;
	uint8_t color;
	uint8_t mirrorMode;
	uint8_t OE;
	uint8_t DA;
	uint8_t rowOrder;
	uint8_t freqPar;
	uint8_t OEWidth;
	uint8_t OEAngle;
	uint8_t faultProcessMode;
	uint8_t commTimeout;
	uint8_t runMode;
	uint8_t logMode;
	uint8_t reverse[8];
	uint16_t CHK;
}cmd_item_controller_params_t;
typedef struct
{//图文内容
	uint8_t* data;
	uint8_t dataLen;
}cmd_picAndtext_content_t;
typedef struct
{//区域内容格式
	uint8_t areaType;
	uint16_t areaX;
	uint16_t areaY;
	uint16_t areaWidth;
	uint16_t areaHeight;
	uint8_t dynamicAreaLoc;
	uint8_t lines_sizes;
	uint8_t runMode;
	uint16_t timeout;
	uint8_t soundMode;
	uint16_t reserved;
	uint8_t singleLine;
	uint8_t newline;
	uint8_t displayMode;
	uint8_t exitMode;
	uint8_t speed;
	uint8_t stayTime;
	uint32_t dataLen;
	cmd_picAndtext_content_t* picAndtext_content;//图文内容	
}cmd_area_data_t;
typedef struct
{//节目文件格式
	uint8_t fileType;
	uint8_t fileName[4];
	uint32_t fileLen;
	uint8_t priority;
	uint16_t displayType;
	uint8_t playTimes;
	uint8_t programLife[8];
	uint8_t programWeek;
	uint8_t programTime;
	uint8_t playPeriodGrpNum;
	//uint8_t playPeriodSetting0[6];
	uint8_t areaNum;//目前区域个数为3
	uint32_t areaDataLen0;
	cmd_area_data_t *areaData0;
	uint32_t areaDataLen1;
	cmd_area_data_t *areaData1;
	uint32_t areaDataLen2;
	cmd_area_data_t *areaData2;	
	uint16_t CHK;	
}cmd_item_data_t;
typedef struct
{//开始写命令
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint8_t overWrite;
	uint8_t fileName[4];
	uint32_t fileLength;
}cmd_dataField_startWirte_t;
typedef struct
{//写命令
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint8_t fileName[4];
	uint8_t lastBlockFlag;
	uint16_t blockNum;
	uint16_t blockLen;
	uint32_t blockAddr;
	cmd_item_data_t *item_data;
}cmd_dataField_write_t;
typedef struct
{//ping命令
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint8_t DeleteAreaNum;
	uint8_t AreaNum;
	uint16_t areaDataLen0;
	cmd_area_data_t *areaData0;
	uint16_t areaDataLen1;
	cmd_area_data_t *areaData1;
	uint16_t areaDataLen2;
	cmd_area_data_t *areaData2;
}cmd_rtShow_content_t;
typedef struct
{//开关机
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint8_t OnOffFlag;
}cmd_dataField_on_off_t;
typedef struct
{// 删除文件
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint16_t FileNumber;
}cmd_dataField_del_file_t;
typedef struct
{//ping命令
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
}cmd_dataField_ping_t;
typedef struct
{//写配置文件命令
	uint8_t cmdGroup;
	uint8_t cmd;
	uint8_t response;
	uint16_t reserved;
	uint8_t fileName[4];
	uint8_t lastBlockFlag;
	uint16_t blockNum;
	uint16_t blockLen;
	uint32_t blockAddr;
	cmd_item_controller_params_t controller_params;
}cmd_dataField_config_t;
typedef struct
{//包头
	uint16_t dstAddr;
	uint16_t srcAddr;
	uint8_t reserved[5];
	uint8_t displayMode;
	uint8_t deviceType;
	uint8_t protocolVersion;
	uint16_t dataLen;
}cmd_packetHeader_t;
typedef struct
{//包尾
	uint16_t crc16;
}cmd_packetTail_t;
typedef struct
{//帧头
	uint8_t frameHeader[8];
}cmd_frameHeader_t;
typedef struct
{//帧尾
	uint8_t frameTail;
}cmd_frameTail_t;

//工程中用到命令如下
typedef struct
{//ping命令
	cmd_frameHeader_t    frameHeader;
	cmd_packetHeader_t   packetHeader;
	cmd_dataField_on_off_t dataField_on_off;
	cmd_packetTail_t       packetTail;
	cmd_frameTail_t        frameTail;	
}cmd_on_off_t;
typedef struct
{//删除文件
	cmd_frameHeader_t         frameHeader;
	cmd_packetHeader_t        packetHeader;
	cmd_dataField_del_file_t  dataField_del_file;
	cmd_packetTail_t          packetTail;
	cmd_frameTail_t           frameTail;
}cmd_del_file_t;
typedef struct
{//ping命令
	cmd_frameHeader_t    frameHeader;
	cmd_packetHeader_t   packetHeader;
	cmd_dataField_ping_t dataField_ping;
	cmd_packetTail_t     packetTail;
	cmd_frameTail_t      frameTail;
}cmd_ping_t;
typedef struct
{//开始写命令
	cmd_frameHeader_t          frameHeader;
	cmd_packetHeader_t         packetHeader;
	cmd_dataField_startWirte_t dataField_startWrite;
	cmd_packetTail_t           packetTail;
	cmd_frameTail_t            frameTail;
}cmd_startWrite_t;
typedef struct
{//写命令
	cmd_frameHeader_t     frameHeader;
	cmd_packetHeader_t    packetHeader;
	cmd_dataField_write_t dataField_write;
	cmd_packetTail_t      packetTail;
	cmd_frameTail_t       frameTail;
}cmd_write_t;
typedef struct
{//写配置文件
	cmd_frameHeader_t	   frameHeader;
	cmd_packetHeader_t	   packetHeader;
	cmd_dataField_config_t dataField_config;
	cmd_packetTail_t       packetTail;
	cmd_frameTail_t        frameTail;
}cmd_config_t;
typedef struct
{//发送实时显示内容
	cmd_frameHeader_t	   frameHeader;
	cmd_packetHeader_t	   packetHeader;
	cmd_rtShow_content_t   rtShow_content;
	cmd_packetTail_t       packetTail;
	cmd_frameTail_t        frameTail;
}cmd_rtShow_t;
#pragma pack()

typedef enum
{
	LED_NULL=0x00,
	LED_CONFIG,
	LED_SHOW
}led_event_type;
//void led_add_event(led_event_type event_type);


typedef enum
{
	TYPE_9600 = 0,
	TYPE_FULL
}LED_config_range_t;

static char g_public_buffer[600];
static uint8_t g_getLedAck = 0;


static char str_winddirect[10];
static char* d2str_winddirect(uint16_t w)
{
    memset(str_winddirect, 0, sizeof(str_winddirect));
    if((w>348)||(w<=11))
        memcpy(str_winddirect, "北风", sizeof("北风"));
	else if((w>11)&&(w<=78))
        memcpy(str_winddirect, "东北风", sizeof("东北风"));
	else if((w>78)&&(w<=101))
        memcpy(str_winddirect, "东风", sizeof("东风"));
	else if((w>101)&&(w<=168))
        memcpy(str_winddirect, "东南风", sizeof("东南风"));
	else if((w>168)&&(w<=191))
        memcpy(str_winddirect, "南风", sizeof("南风"));
	else if((w>191)&&(w<=258))
        memcpy(str_winddirect, "西南风", sizeof("西南风"));
	else if((w>258)&&(w<=281))
        memcpy(str_winddirect, "西风", sizeof("西风"));
	else if((w>281)&&(w<=348))
        memcpy(str_winddirect, "西北风", sizeof("西北风"));
    else
        memcpy(str_winddirect, "北风", sizeof("北风"));
	return str_winddirect;
}
const uint16_t table[256] = 
{      
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,     
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,     
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,      
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,     
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,       
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,     
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,     
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,     
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,     
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,        
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,     
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,     
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,     
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,     
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,        
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,     
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,     
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,     
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,     
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,        
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,     
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,     
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,     
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,     
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,       
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,     
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,     
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,     
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,     
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,       
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,     
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};
static void ledCRC16(const uint8_t* pDataIn, int iLenIn, uint16_t* pCRCOut)     
{     
    uint16_t wResult = 0;     
    uint16_t wTableNo = 0;  
	uint16_t i;   
    
    for(i = 0; i < iLenIn; i++)     
    {     
        wTableNo = ((wResult & 0xff) ^ (pDataIn[i] & 0xff));     
        wResult = ((wResult >> 8) & 0xff) ^ table[wTableNo]; 
		//SysPrintf("%02x ",pDataIn[i]);		
    }     
    //SysPrintf("\n");
    *pCRCOut = wResult;     
} 


static void LED_content_transfer(uint8_t *buf,uint16_t *len);

void LED_ping(uint16_t mainboard_id,uint16_t controller_id)
{
	cmd_ping_t *p_cmd_ping;
	memset(g_public_buffer,0,sizeof(g_public_buffer));
	p_cmd_ping = (cmd_ping_t*)g_public_buffer;
	memset((uint8_t*)&p_cmd_ping->frameHeader,0xa5,sizeof(p_cmd_ping->frameHeader));
	p_cmd_ping->packetHeader.dstAddr         = controller_id;
	p_cmd_ping->packetHeader.srcAddr         = mainboard_id;
	//p_cmd_ping->packetHeader.reserved        = 0L;
	memset((uint8_t*)&p_cmd_ping->packetHeader.reserved,0x00,sizeof(p_cmd_ping->packetHeader.reserved));
	p_cmd_ping->packetHeader.displayMode     = 0;
	p_cmd_ping->packetHeader.deviceType      = 0xfe;
	p_cmd_ping->packetHeader.protocolVersion = 0x02;
	p_cmd_ping->packetHeader.dataLen         = 0x05;
	p_cmd_ping->dataField_ping.cmdGroup = 0xa2;
	p_cmd_ping->dataField_ping.cmd      = 0x00;
	p_cmd_ping->dataField_ping.response = 0x01;
	p_cmd_ping->dataField_ping.reserved = 0;
	//uint16_t crc16;
	//crc16 = Modbus_CRC16((uint8_t*)&(p_cmd_ping->packetHeader),sizeof(p_cmd_ping->packetHeader)+sizeof(p_cmd_ping->dataField_ping));
	ledCRC16((const uint8_t*)&(p_cmd_ping->packetHeader),sizeof(p_cmd_ping->packetHeader)+sizeof(p_cmd_ping->dataField_ping),&(p_cmd_ping->packetTail.crc16));
	//SysPrintf("ping modbus crc16 is %x\n",p_cmd_ping->packetTail.crc16);
	
	//p_cmd_ping->packetTail.crc[0]            = 0x68;
	//p_cmd_ping->packetTail.crc[1]            = 0xf8;
	p_cmd_ping->frameTail.frameTail = 0x5a;
	hal_ledpanel_com_send((uint8_t*)p_cmd_ping,sizeof(cmd_ping_t));
}
void LED_startWrite_configFile(uint16_t mainboard_id,uint16_t controller_id)
{
    int i = 0x1ff;
    while(i-- >0);
    
    cmd_startWrite_t *p_cmd_startWrite;
	memset(g_public_buffer,0,sizeof(g_public_buffer));
	p_cmd_startWrite = (cmd_startWrite_t*)g_public_buffer;
	memset((uint8_t*)&p_cmd_startWrite->frameHeader,0xa5,sizeof(p_cmd_startWrite->frameHeader));
	p_cmd_startWrite->packetHeader.dstAddr         = controller_id;
	p_cmd_startWrite->packetHeader.srcAddr         = mainboard_id;
	//p_cmd_startWrite->packetHeader.reserved        = 0;
	memset((uint8_t*)&p_cmd_startWrite->packetHeader.reserved,0x00,sizeof(p_cmd_startWrite->packetHeader.reserved));
	p_cmd_startWrite->packetHeader.displayMode     = 0;
	p_cmd_startWrite->packetHeader.deviceType      = 0xfe;
	p_cmd_startWrite->packetHeader.protocolVersion = 0x02;
	p_cmd_startWrite->packetHeader.dataLen         = 0x0e;
	p_cmd_startWrite->dataField_startWrite.cmdGroup = 0xa1;
	p_cmd_startWrite->dataField_startWrite.cmd = 0x05;
	p_cmd_startWrite->dataField_startWrite.response = 0x01;
	p_cmd_startWrite->dataField_startWrite.reserved = 0;
	p_cmd_startWrite->dataField_startWrite.overWrite = 1;
	const char fileName[]="C000";
	//p_cmd_startWrite->dataField_startWrite.fileName = "C000";
	memcpy(p_cmd_startWrite->dataField_startWrite.fileName,fileName,strlen(fileName));
	p_cmd_startWrite->dataField_startWrite.fileLength = 0x2e;

	ledCRC16((const uint8_t*)&(p_cmd_startWrite->packetHeader),sizeof(p_cmd_startWrite->packetHeader)+sizeof(p_cmd_startWrite->dataField_startWrite),&(p_cmd_startWrite->packetTail.crc16));
	//SysPrintf("p_cmd_startWrite crc16 is %x\n",p_cmd_startWrite->packetTail.crc16);
	//p_cmd_startWrite->packetTail.crc[0]            = 0xbc;
	//p_cmd_startWrite->packetTail.crc[1]            = 0x7b;
	p_cmd_startWrite->frameTail.frameTail = 0x5a;
	hal_ledpanel_com_send((uint8_t*)p_cmd_startWrite,sizeof(cmd_startWrite_t));
		
}
void LED_write_configFile(uint16_t mainboard_id,uint16_t controller_id)
{
    int i = 0x1ff;
    while(i-- >0);

    cmd_config_t *p_cmd_config;
	memset(g_public_buffer,0,sizeof(g_public_buffer));
	p_cmd_config = (cmd_config_t*)g_public_buffer;
	memset((uint8_t*)&p_cmd_config->frameHeader,0xa5,sizeof(p_cmd_config->frameHeader));
	p_cmd_config->packetHeader.dstAddr         = controller_id;
	p_cmd_config->packetHeader.srcAddr         = mainboard_id;
	//p_cmd_config->packetHeader.reserved        = 0;
	memset((uint8_t*)&p_cmd_config->packetHeader.reserved,0x00,sizeof(p_cmd_config->packetHeader.reserved));
	p_cmd_config->packetHeader.displayMode     = 0;
	p_cmd_config->packetHeader.deviceType      = 0xfe;
	p_cmd_config->packetHeader.protocolVersion = 0x02;
	p_cmd_config->packetHeader.dataLen         = 0x40;
	p_cmd_config->dataField_config.cmdGroup = 0xa1;
	p_cmd_config->dataField_config.cmd = 0x06;
	p_cmd_config->dataField_config.response = 0x01;
	p_cmd_config->dataField_config.reserved = 0;
	p_cmd_config->dataField_config.fileName[0] = 0x43;
	p_cmd_config->dataField_config.fileName[1] = 0x30;
	p_cmd_config->dataField_config.fileName[2] = 0x30;
	p_cmd_config->dataField_config.fileName[3] = 0x30;
	p_cmd_config->dataField_config.lastBlockFlag = 0x01;
	p_cmd_config->dataField_config.blockNum = 0x00;
	p_cmd_config->dataField_config.blockLen = 0x2e;
	p_cmd_config->dataField_config.blockAddr = 0x00;
	p_cmd_config->dataField_config.controller_params.fileType = 0x01;
	const char fileName[]="C000";
	//p_cmd_config->dataField_config.controller_params.fileName = "C000";
	memcpy(p_cmd_config->dataField_config.controller_params.fileName,fileName,strlen(fileName));
	p_cmd_config->dataField_config.controller_params.fileLen = 0x2e;
	const char controlName[]="LED00002";
	//p_cmd_config->dataField_config.controller_params.controlName = "LED00001";
	memcpy(p_cmd_config->dataField_config.controller_params.controlName,controlName,strlen(controlName));
	p_cmd_config->dataField_config.controller_params.address = 0x02;
	p_cmd_config->dataField_config.controller_params.baudrate = 0x01;//9600
	p_cmd_config->dataField_config.controller_params.screenWidth = 0x40;
	p_cmd_config->dataField_config.controller_params.screenHeight = 0x30;
 	p_cmd_config->dataField_config.controller_params.color = 0x01;
	p_cmd_config->dataField_config.controller_params.mirrorMode = 0;
	p_cmd_config->dataField_config.controller_params.OE = 1;
	p_cmd_config->dataField_config.controller_params.DA = 0;
	p_cmd_config->dataField_config.controller_params.rowOrder = 0;
	p_cmd_config->dataField_config.controller_params.freqPar = 0;
	p_cmd_config->dataField_config.controller_params.OEWidth = 0x0c;
	p_cmd_config->dataField_config.controller_params.OEAngle = 0;
	p_cmd_config->dataField_config.controller_params.faultProcessMode = 0;
	p_cmd_config->dataField_config.controller_params.commTimeout = 5;
	p_cmd_config->dataField_config.controller_params.runMode = 0;
	p_cmd_config->dataField_config.controller_params.logMode = 0;
	//p_cmd_config->dataField_config.controller_params.reverse = 0;
	memset((uint8_t*)&p_cmd_config->dataField_config.controller_params.reverse,0x00,sizeof(p_cmd_config->dataField_config.controller_params.reverse));
	ledCRC16((const uint8_t*)&(p_cmd_config->dataField_config.controller_params.fileType),p_cmd_config->dataField_config.controller_params.fileLen-2,&(p_cmd_config->dataField_config.controller_params.CHK));
	//SysPrintf("CHK is %x\n",p_cmd_config->dataField_config.controller_params.CHK);
	//p_cmd_config->dataField_config.controller_params.CHK[0] = 0x5d;
	//p_cmd_config->dataField_config.controller_params.CHK[1] = 0x75;

	ledCRC16((const uint8_t*)&(p_cmd_config->packetHeader),sizeof(p_cmd_config->packetHeader)+sizeof(p_cmd_config->dataField_config),&(p_cmd_config->packetTail.crc16));
	//SysPrintf("p_cmd_startWrite crc16 is %x\n",p_cmd_config->packetTail.crc16);
	//p_cmd_config->packetTail.crc[0] = 0x7e;
	//p_cmd_config->packetTail.crc[1] = 0xa4;
	p_cmd_config->frameTail.frameTail = 0x5a;
	uint16_t len = sizeof(cmd_config_t);
	LED_content_transfer((uint8_t*)p_cmd_config,&len);
	hal_ledpanel_com_send((uint8_t*)p_cmd_config,len);
}

typedef enum
{
	LED_RATE_57600_ID_1,
	LED_RATE_57600_ID_2,
	LED_RATE_9600_ID_1,
	LED_RATE_9600_ID_2,
	LED_UNKNOWN
}LED_controller_param_t;

typedef struct
{
    char* content;
    char* formate;
    uint8_t dunum;//display units nums
    int dubuf[5]; //display unit ID   
}led_line_formate_t;

typedef struct
{
    led_line_formate_t led_line_display[3];
    char* content[3];
    uint8_t refresh;
}led_panel_params_t;
static led_panel_params_t led_panel_params; 

void import_args_into_buf(int* buf, int num, ...)
{
    uint8_t i = 0;
    va_list list;
	va_start(list, num);
    while(i < num)
    {
        int tmp = va_arg(list, int);
        buf[i] = tmp;
        i++;
    }
    va_end(list);
}

#define led_line_display_config(type, line, unitsNum, ...)  {\
    led_panel_params.led_line_display[line].content = type##_ledcontent_line##line;\
    led_panel_params.led_line_display[line].formate = type##_lcformate_line##line;\
    led_panel_params.led_line_display[line].dunum = unitsNum;\
    import_args_into_buf(led_panel_params.led_line_display[line].dubuf, unitsNum,  ##__VA_ARGS__);\
}

typedef enum
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STR
}arg_type_t;

typedef struct
{
    int val_int;
    float val_float;
    char* val_str;
    arg_type_t type;
}arg_val_t;

arg_val_t get_showvalue_by_id(int id)
{
    arg_val_t arg;
    arg.type = TYPE_INT;
    float value = SensorGetRealValue((YMSensorType_t)id);
    switch(id)
    {
        case YM_SENSOR_TYPE_PM25:
        case YM_SENSOR_TYPE_PM10:
            arg.val_int = ((int)value>999)?999:(int)value;
            break;
        case YM_SENSOR_TYPE_WD:
            arg.val_str = d2str_winddirect((uint16_t)value);
            arg.type = TYPE_STR;
            break;
        case YM_SENSOR_TYPE_CO:
        {
            if(value < 0.001)
                arg.val_float = (float)(rand()%10)/100.0;
            else
                arg.val_float = (value>9.99)?9.99:value;
            arg.type = TYPE_FLOAT;
            break;
        }
        case YM_SENSOR_TYPE_VOC:
        {
            if(value < 0.001)
                arg.val_float = (float)(rand()%10)/100.0;
            else
                arg.val_float = (value>99.99)?99.99:value;
            arg.type = TYPE_FLOAT;
            break;
        }
        case YM_SENSOR_TYPE_NO2:
        case YM_SENSOR_TYPE_SO2:
        case YM_SENSOR_TYPE_O3:
        {
            if(value < 0.001)
                arg.val_int = (rand()%10);
            else
                arg.val_int = ((int)value>9999)?9999:(int)value;
            break;
        }
        default:
            arg.val_int = (int)value;
            break;
    }
    return arg;
}

static char* get_fmt(char* s, char c, uint8_t *l)
{
	char* res=NULL;
    char* q = strchr(s, '%');//fmt必须包含%
    if(q==NULL)
        return NULL;
	char* p = strchr(q, c);
	if(p==NULL)
	{
		res = SysMalloc(strlen(s)+1);
		memset(res, 0, strlen(s)+1);
		memcpy(res, s, strlen(s));
		return res;
	}
	while(*p == c)
        p++;
	res = SysMalloc(p-s+1);
	memset(res, 0, p-s+1);
	memcpy(res, s, p-s);
    *l = p-s;
	return res;
}

static cmd_picAndtext_content_t* build_picAndtext_line_content(int line)
{
	cmd_picAndtext_content_t *p_cmd_picAndtext_content = NULL;
	p_cmd_picAndtext_content = SysMalloc(sizeof(cmd_picAndtext_content_t));
	if(p_cmd_picAndtext_content == NULL)
	{
		SysPrintf("malloc failed\n");
		return NULL;
	}
	p_cmd_picAndtext_content->dataLen = strlen(led_panel_params.led_line_display[line].content);
	p_cmd_picAndtext_content->data = SysMalloc(p_cmd_picAndtext_content->dataLen+1);
	if(p_cmd_picAndtext_content->data == NULL)
	{
		SysPrintf("malloc failed\n");
        free(p_cmd_picAndtext_content);
		return NULL;
	}
    memset(p_cmd_picAndtext_content->data, 0, p_cmd_picAndtext_content->dataLen+1);
    arg_val_t* new_args = SysMalloc(led_panel_params.led_line_display[line].dunum * sizeof(arg_val_t));
    if(new_args == NULL)
    {
        SysPrintf("malloc failed\n");
        free(p_cmd_picAndtext_content->data);
        free(p_cmd_picAndtext_content);
        return NULL;
    }
    memset(new_args, 0, led_panel_params.led_line_display[line].dunum * sizeof(arg_val_t));
    int formate_len = strlen(led_panel_params.led_line_display[line].formate);
    char* fmt_tmp = SysMalloc(formate_len+1);
	memset(fmt_tmp, 0, formate_len+1);
	memcpy(fmt_tmp, led_panel_params.led_line_display[line].formate, formate_len);
    uint8_t i = 0;
    int fmt_i = 0;
    while(i < led_panel_params.led_line_display[line].dunum)
    {
        new_args[i] = get_showvalue_by_id(led_panel_params.led_line_display[line].dubuf[i]);
        uint8_t len = 0;
        char* fmt = get_fmt(fmt_tmp+fmt_i, ' ', &len);
        if(fmt==NULL)
        {
            SysPrintf("fmt error\n");
            return NULL;
        }
        fmt_i += len;
        char tmp[50];
        memset(tmp, 0, 50);
        switch(new_args[i].type)
        {
            case TYPE_INT:
                sprintf(tmp, fmt, new_args[i].val_int); 
                //printf("+%s\n", fmt);
                //printf(fmt, new_args[i].val_int);
                break;
            case TYPE_FLOAT:
                sprintf(tmp, fmt, new_args[i].val_float);
                //printf("+%s\n", fmt);
                //printf(fmt, new_args[i].val_float);
                break;
            case TYPE_STR:
                sprintf(tmp, fmt, new_args[i].val_str);
                //printf("+%s\n", fmt);
                //printf(fmt, new_args[i].val_str);
                break;
            default:
                break;
        }
        strcat((char *)p_cmd_picAndtext_content->data, tmp);

		free(fmt);
        i++;
    };
    free(fmt_tmp);
    free(new_args);
    printf("%s\n", p_cmd_picAndtext_content->data);
	return p_cmd_picAndtext_content;    
}

static cmd_area_data_t* build_area_line_data(int line, int x, int y)
{
	//SysPrintf("build_area0_data\n");
	cmd_picAndtext_content_t *p_cmd_picAndtext_content;
	p_cmd_picAndtext_content = build_picAndtext_line_content(line);
	if(p_cmd_picAndtext_content == NULL)
	{
		return NULL;
	}
	cmd_area_data_t *p_cmd_area_data;
	p_cmd_area_data = SysMalloc(sizeof(cmd_area_data_t));
	if(p_cmd_area_data == NULL)
	{
		SysPrintf("malloc failed\n");
		free(p_cmd_picAndtext_content->data);
        free(p_cmd_picAndtext_content);
		return NULL;
	}
	p_cmd_area_data->picAndtext_content = p_cmd_picAndtext_content;
	p_cmd_area_data->areaType       = 0x00;
	p_cmd_area_data->areaX          = x;
	p_cmd_area_data->areaY          = y;
	p_cmd_area_data->areaWidth      = 0x08;
	p_cmd_area_data->areaHeight     = 0x10;
	p_cmd_area_data->dynamicAreaLoc = line;
	p_cmd_area_data->lines_sizes    = 0x00;
	p_cmd_area_data->runMode        = 0x00;
	p_cmd_area_data->timeout        = 0x00;
	p_cmd_area_data->soundMode      = 0x00;
	p_cmd_area_data->reserved       = 0x00;
	p_cmd_area_data->singleLine     = 0x01;
	p_cmd_area_data->newline        = 0x02;
	p_cmd_area_data->displayMode    = 0x03;
	p_cmd_area_data->exitMode       = 0x00;
	p_cmd_area_data->speed          = 0x18;
	p_cmd_area_data->stayTime       = 0x00;
	p_cmd_area_data->dataLen        = p_cmd_area_data->picAndtext_content->dataLen;
	return p_cmd_area_data; 			
}
static void free_area_line_data(cmd_area_data_t* p)
{
    if(p->picAndtext_content)
    {
        free(p->picAndtext_content->data);
        free(p->picAndtext_content);
    }

    free(p);
}
static cmd_rtShow_content_t* build_rtShow_content()
{
	cmd_area_data_t *p_cmd_area0_data;
	p_cmd_area0_data = build_area_line_data(0, 0, 0);
	if(p_cmd_area0_data == NULL)
	{
		return NULL;
	}
	cmd_area_data_t *p_cmd_area1_data;
	p_cmd_area1_data = build_area_line_data(1, 0, 0x10);
	if(p_cmd_area1_data == NULL)
	{
        free_area_line_data(p_cmd_area0_data);
        return NULL;
	}
	cmd_area_data_t *p_cmd_area2_data;
	p_cmd_area2_data = build_area_line_data(2, 0, 0x20);
	if(p_cmd_area2_data == NULL)
	{
        free_area_line_data(p_cmd_area0_data);
        free_area_line_data(p_cmd_area1_data);
        return NULL;
	}
	cmd_rtShow_content_t * p_rtShow_content = NULL;
	p_rtShow_content = SysMalloc(sizeof(cmd_rtShow_content_t));
	if(p_rtShow_content == NULL)
	{
		SysPrintf("malloc failed\n");
        free_area_line_data(p_cmd_area0_data);
        free_area_line_data(p_cmd_area1_data);
        free_area_line_data(p_cmd_area2_data);
		return NULL;
	}
	p_rtShow_content->areaData0 = p_cmd_area0_data;
	p_rtShow_content->areaData1 = p_cmd_area1_data;
	p_rtShow_content->areaData2 = p_cmd_area2_data;
	p_rtShow_content->areaDataLen0 = sizeof(cmd_area_data_t)-4+p_cmd_area0_data->picAndtext_content->dataLen;	
	p_rtShow_content->areaDataLen1 = sizeof(cmd_area_data_t)-4+p_cmd_area1_data->picAndtext_content->dataLen;
	p_rtShow_content->areaDataLen2 = sizeof(cmd_area_data_t)-4+p_cmd_area2_data->picAndtext_content->dataLen;
	p_rtShow_content->cmdGroup      = 0xa3;
	p_rtShow_content->cmd           = 0x06;
	p_rtShow_content->response      = 0x01;
	p_rtShow_content->reserved      = 0;
	p_rtShow_content->DeleteAreaNum = 0;
	p_rtShow_content->AreaNum       = 0x03;
	memset((uint8_t*)g_public_buffer,0x00,sizeof(g_public_buffer));
	uint16_t i_juncture = sizeof(cmd_frameHeader_t) + sizeof(cmd_packetHeader_t);
	memcpy((uint8_t*)g_public_buffer+i_juncture,(uint8_t*)p_rtShow_content,sizeof(cmd_rtShow_content_t)-(4+2)*3);
	i_juncture += (sizeof(cmd_rtShow_content_t)-(4+2)*3);
	memcpy((uint8_t*)g_public_buffer+i_juncture,&p_rtShow_content->areaDataLen0,sizeof(p_rtShow_content->areaDataLen0));
	i_juncture += sizeof(p_rtShow_content->areaDataLen0);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData0,sizeof(cmd_area_data_t)-4);
	i_juncture += (sizeof(cmd_area_data_t)-4);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData0->picAndtext_content->data,p_rtShow_content->areaData0->picAndtext_content->dataLen);
	i_juncture += p_rtShow_content->areaData0->picAndtext_content->dataLen;
	memcpy((uint8_t*)g_public_buffer+i_juncture,&p_rtShow_content->areaDataLen1,sizeof(p_rtShow_content->areaDataLen1));
	i_juncture += sizeof(p_rtShow_content->areaDataLen1);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData1,sizeof(cmd_area_data_t)-4);
	i_juncture += (sizeof(cmd_area_data_t)-4);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData1->picAndtext_content->data,p_rtShow_content->areaData1->picAndtext_content->dataLen);
	i_juncture += p_rtShow_content->areaData1->picAndtext_content->dataLen;
	memcpy((uint8_t*)g_public_buffer+i_juncture,&p_rtShow_content->areaDataLen2,sizeof(p_rtShow_content->areaDataLen2));
	i_juncture += sizeof(p_rtShow_content->areaDataLen2);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData2,sizeof(cmd_area_data_t)-4);
	i_juncture += (sizeof(cmd_area_data_t)-4);
	memcpy((uint8_t*)g_public_buffer+i_juncture,p_rtShow_content->areaData2->picAndtext_content->data,p_rtShow_content->areaData2->picAndtext_content->dataLen);
	i_juncture += p_rtShow_content->areaData2->picAndtext_content->dataLen;
	return p_rtShow_content;
}

static void LED_content_transfer(uint8_t *buf,uint16_t *len)
{
	uint16_t start_pos = 8;
	uint16_t i = 0,j = 0;
	for(i=start_pos;i<*len-1;i++)
	{
		if(buf[i] == 0x5a)
		{
			for(j=*len;j>i;j--)
			{
				buf[j] = buf[j-1];
			}
			buf[i] = 0x5b;buf[i+1] = 0x02;
			*len+=1;
		}
		else if(buf[i] == 0xa5)
		{
			for(j=*len;j>i;j--)
			{
				buf[j] = buf[j-1];
			}
			buf[i] = 0xa6;buf[i+1] = 0x02;
			*len+=1;
		}
		else if((buf[i] == 0x5b)&&(buf[i+1] != 0x02))
		{
			for(j=*len;j>i;j--)
			{
				buf[j] = buf[j-1];
			}
			buf[i] = 0x5b;buf[i+1] = 0x01;
			*len+=1;
		}
		else if((buf[i] == 0xa6)&&(buf[i+1] != 0x02))
		{
			for(j=*len;j>i;j--)
			{
				buf[j] = buf[j-1];
			}
			buf[i] = 0xa6;buf[i+1] = 0x01;
			*len+=1;
		}
	}
}

void LED_rt_show(uint16_t mainboard_id,uint16_t controller_id)
{
	cmd_rtShow_content_t *p_rtShow_content;
	p_rtShow_content = build_rtShow_content();
	if(p_rtShow_content == NULL)
	{
		return;
	}
	cmd_packetHeader_t cmd_packetHeader;
	cmd_packetHeader.dstAddr = controller_id;
	cmd_packetHeader.srcAddr = mainboard_id;
	memset((uint8_t*)cmd_packetHeader.reserved,0x00,sizeof(cmd_packetHeader.reserved));
	cmd_packetHeader.displayMode = 0x01;
	cmd_packetHeader.deviceType = 0xfe;
	cmd_packetHeader.protocolVersion = 0x02;
	cmd_packetHeader.dataLen = sizeof(cmd_rtShow_content_t)-4*3+p_rtShow_content->areaDataLen0 + p_rtShow_content->areaDataLen1 + p_rtShow_content->areaDataLen2;/*0x12 + p_cmd_dataField_write->item_data->fileLen;*/
	cmd_frameHeader_t cmd_frameHeader;
	memset((uint8_t*)&cmd_frameHeader,0xa5,sizeof(cmd_frameHeader_t));
	cmd_packetTail_t cmd_packetTail;
	cmd_frameTail_t cmd_frameTail;
	cmd_frameTail.frameTail = 0x5a;
	uint16_t i_juncture = 0;
	memcpy((uint8_t*)g_public_buffer,(uint8_t*)&cmd_frameHeader,sizeof(cmd_frameHeader));
	i_juncture += sizeof(cmd_frameHeader);
	memcpy((uint8_t*)g_public_buffer+i_juncture,(uint8_t*)&cmd_packetHeader,sizeof(cmd_packetHeader_t));
    i_juncture += sizeof(cmd_packetHeader_t);
	i_juncture += cmd_packetHeader.dataLen;
	ledCRC16((const uint8_t*)g_public_buffer+sizeof(cmd_frameHeader_t),i_juncture-sizeof(cmd_frameHeader_t),&(cmd_packetTail.crc16));
    memcpy((uint8_t*)g_public_buffer+i_juncture,(uint8_t*)&cmd_packetTail,sizeof(cmd_packetTail_t));
    i_juncture += sizeof(cmd_packetTail_t);
	memcpy((uint8_t*)g_public_buffer+i_juncture,(uint8_t*)&cmd_frameTail,sizeof(cmd_frameTail_t));
    i_juncture += sizeof(cmd_frameTail_t);
	uint16_t len = i_juncture;
	LED_content_transfer((uint8_t*)g_public_buffer,&len);
	//comSendBuf(1,(uint8_t*)g_public_buffer,len);
	//int i=0;
    //while(i<len)
    //    printf("%x ", g_public_buffer[i++]);
	hal_ledpanel_com_send((uint8_t*)g_public_buffer,len);
    if(p_rtShow_content->areaData0)
        free_area_line_data(p_rtShow_content->areaData0);
    if(p_rtShow_content->areaData1)
        free_area_line_data(p_rtShow_content->areaData1);
    if(p_rtShow_content->areaData2)
        free_area_line_data(p_rtShow_content->areaData2);
	if(p_rtShow_content)
		free(p_rtShow_content);
}

extern uint8_t g_getLedAck;

int led_config(LED_config_range_t type)
{
	//LED_controller_param_t controller_param = LED_UNKNOWN;
	static uint8_t pending = 0;
	static SysTime_t rt_timer;
	if(0 == pending)
	{
		hal_ledpanel_com_config(9600);
		LED_ping(0x8000,0x01);
		pending = 1;//SysPrintf("pending = 1\n");
		rt_timer = SysTime();
		return pending;
	}
	if(1 == pending)
	{
		if(g_getLedAck)
		{
			pending = 2;//SysPrintf("pending = 2\n");
			g_getLedAck = 0;
			//controller_param = LED_RATE_9600_ID_1;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			LED_ping(0x8000,0x02);
			pending = 3;//SysPrintf("pending = 3\n");
			rt_timer = SysTime();
		}
		return pending;
	}
	if(2 == pending)
	{
		//9600 ping 01 ok
	}
	if(3 == pending)
	{
		if(g_getLedAck)
		{
			pending = 4;//SysPrintf("pending = 4\n");
			g_getLedAck = 0;
			//controller_param = LED_RATE_9600_ID_2;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{// bardrate need change
			SysPrintf("type = %d\n",type);
			if(type == TYPE_FULL)
			{
				hal_ledpanel_com_config(57600);
				LED_ping(0x8000,0x01);
				pending = 5;//SysPrintf("pending = 5\n");
				rt_timer = SysTime();
				return pending;
			}
			else
			{
				pending = 0;
				SysPrintf("can not confirm controller param\n");
				return -1;
			}
		}
		return pending;
	}
	if(4 == pending)
	{
		//9600 ping 02 ok
	}
	if(5 == pending)
	{
		if(g_getLedAck)
		{
			pending = 6;//SysPrintf("pending = 6\n");
			g_getLedAck = 0;
			//controller_param = LED_RATE_57600_ID_1;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			LED_ping(0x8000,0x02);
			pending = 7;//SysPrintf("pending = 7\n");
			rt_timer = SysTime();
		}
		return pending;
	}
	if(6 == pending)
	{
		//57600 ping 01 ok
	}
	if(7 == pending)
	{
		if(g_getLedAck)
		{
			pending = 8;//SysPrintf("pending = 8\n");
			g_getLedAck = 0;
			//controller_param = LED_RATE_57600_ID_2;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("can not confirm controller param\n");
			hal_ledpanel_com_config(9600);
			return -1;
		}
		return pending;
	}
	if(8 == pending)
	{
		//57600 ping 02 ok
	}
	//---------------------
	if(2 == pending)
	{
		//9600 ping 01 ok
		LED_startWrite_configFile(0x8000,0x01);
		pending = 9;//SysPrintf("pending = 9\n");
		rt_timer = SysTime();
		return pending;
	}
	if(9 == pending)
	{
		if(g_getLedAck)
		{
			pending = 10;//SysPrintf("pending = 10\n");
			g_getLedAck = 0;
			SysPrintf("9600LED_write_configFile\n");
			LED_write_configFile(0x8000,0x01);
			rt_timer = SysTime();
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_startWrite_configFile fail\n");
			return -1;
		}
		return pending;
	}
	if(10 == pending)
	{
		if(g_getLedAck)
		{
			pending = 0;
			g_getLedAck = 0;
			SysPrintf("LED_RATE_9600_ID_1 success\n");
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_RATE_9600_ID_1 fail\n");
			return -1;
		}
		return pending;
	}
	//---------------------
	//---------------------
	if(4 == pending)
	{
		//9600 ping 02 ok
		LED_startWrite_configFile(0x8000,0x02);
		pending = 11;//SysPrintf("pending = 11\n");
		rt_timer = SysTime();
		return pending;
	}
	if(11 == pending)
	{
		if(g_getLedAck)
		{
			pending = 12;//SysPrintf("pending = 12\n");
			g_getLedAck = 0;
			SysPrintf("9600LED_write_configFile\n");
			LED_write_configFile(0x8000,0x02);
			rt_timer = SysTime();
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_startWrite_configFile fail\n");
			return -1;
		}
		return pending;
	}
	if(12 == pending)
	{
		if(g_getLedAck)
		{
			pending = 0;
			g_getLedAck = 0;
			SysPrintf("LED_RATE_9600_ID_2 success\n");
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_RATE_9600_ID_2 fail\n");
			return -1;
		}
		return pending;
	}
	//---------------------
	//---------------------
	if(6 == pending)
	{
		//57600 ping 01 ok
		LED_startWrite_configFile(0x8000,0x01);
		pending = 13;//SysPrintf("pending = 13\n");
		rt_timer = SysTime();
		return pending;
	}
	if(13 == pending)
	{
		if(g_getLedAck)
		{
			pending = 14;//SysPrintf("pending = 14\n");
			g_getLedAck = 0;
			SysPrintf("9600LED_write_configFile\n");
			LED_write_configFile(0x8000,0x01);
			rt_timer = SysTime();
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_startWrite_configFile fail\n");
			return -1;
		}
		return pending;
	}
	if(14 == pending)
	{
		if(g_getLedAck)
		{
			pending = 0;
			g_getLedAck = 0;
			SysPrintf("LED_RATE_57600_ID_1 success\n");
			hal_ledpanel_com_config(9600);
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_RATE_57600_ID_1 fail\n");
			hal_ledpanel_com_config(9600);
			return -1;
		}
		return pending;
	}
	//---------------------
	//---------------------
	if(8 == pending)
	{
		//57600 ping 02 ok
		LED_startWrite_configFile(0x8000,0x02);
		pending = 15;//SysPrintf("pending = 15\n");
		rt_timer = SysTime();
		return pending;
	}
	if(15 == pending)
	{
		if(g_getLedAck)
		{
			pending = 16;//SysPrintf("pending = 16\n");
			g_getLedAck = 0;
			SysPrintf("57600LED_write_configFile\n");
			LED_write_configFile(0x8000,0x02);
			rt_timer = SysTime();
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_startWrite_configFile fail\n");
			return -1;
		}
		return pending;
	}
	if(16 == pending)
	{
		if(g_getLedAck)
		{
			pending = 0;
			g_getLedAck = 0;
			SysPrintf("LED_RATE_57600_ID_2 success\n");
			hal_ledpanel_com_config(9600);
			return pending;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			pending = 0;
			SysPrintf("LED_RATE_57600_ID_2 fail\n");
			hal_ledpanel_com_config(9600);
			return -1;
		}
		return pending;
	}
	//---------------------
	return 0;	
}

int led_rt_show()
{
	static uint8_t pending = 0;
	static SysTime_t rt_timer;
	if(0 == pending)
	{
		LED_ping(0x8000,0x02);
		pending = 1;//SysPrintf("pending = 1\n");
		rt_timer = SysTime();
		return pending;
	}
	if(1 == pending)
	{
		if((g_getLedAck)&&(SysTimeHasPast(rt_timer,1*1000)))
		{
			pending = 2;//SysPrintf("pending = 2\n");
			g_getLedAck = 0;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			SysPrintf("--led ping not return error\n");
			pending = 0;
			return -1;
		}
		return pending;
	}
	if(2 == pending)
	{
		LED_rt_show(0x8000,0x02);
		pending = 3;//SysPrintf("pending = 3\n");
		rt_timer = SysTime();
		return pending;
	}
	if(3 == pending)
	{
		if(g_getLedAck)
		{
			pending = 0;
			g_getLedAck = 0;
		}
		if(SysTimeHasPast(rt_timer,2*1000))
		{
			SysPrintf("--led rt show error\n");
			pending = 0;
			return -1;
		}
		return pending;
	}
	return 0;
}

#define LEDPANEL_RECV_BUF_LEN 128
static void ledpanel_byte_recv_handler(unsigned char data)
{
	static uint8_t ledpanel_recv_buf[LEDPANEL_RECV_BUF_LEN];
	static uint8_t buf_pos = 0;

	ledpanel_recv_buf[buf_pos++] = data;
	if((buf_pos >= 1)&&(ledpanel_recv_buf[0] != 0xa5))
	{
		buf_pos = 0;
	}
	if(buf_pos >= 30)
	{
		g_getLedAck = 1;
		buf_pos = 0;
	}
	if(buf_pos >= LEDPANEL_RECV_BUF_LEN)
	{
		buf_pos = 0;
	}
}

typedef int (*event_handle)(void* arg);
typedef struct
{
	event_handle handle;
	led_event_type  event_type;
}led_event_t;
static led_event_t g_led_event={NULL,LED_NULL};
void led_add_event(led_event_type event_type)
{
	switch(event_type)
	{
		case LED_CONFIG:
			g_led_event.event_type = LED_CONFIG;
			g_led_event.handle     = (event_handle)led_config;
			break;
		case LED_SHOW:
			g_led_event.event_type = LED_SHOW;
			g_led_event.handle     = (event_handle)led_rt_show;
			break;
		default:
			g_led_event.event_type = (led_event_type)NULL;
			g_led_event.handle     = LED_NULL;
	}
}


typedef enum
{
	LED_STATE_NULL = 0,
	LED_NOT_FOUND,
	LED_NOT_RESPOND,
	LED_OK
}LED_STATE;

void led_panel_poll()
{
	static SysTime_t g_ledshowtimer;
	static SysTime_t g_probetimer;
	static LED_STATE led_state = LED_OK;
	static uint8_t oneshot = 1;
	static uint8_t timer_gap = 0;
	hal_ledpanel_com_recv_poll();
	if(LED_NOT_FOUND == led_state)
	{
		if(!SysTimeHasPast(g_probetimer,2*60*1000))//超过2Min会探测屏幕是否存在
			return;
	}
	if(oneshot)
	{
		led_add_event(LED_CONFIG);
		g_ledshowtimer = SysTime();
		g_probetimer = SysTime();
		oneshot = 0;
	}
	else if(timer_gap)
	{
		if(SysTimeHasPast(g_ledshowtimer,timer_gap*1000))
		{
			if(LED_OK == led_state)
				led_add_event(LED_SHOW);
			else
				led_add_event(LED_CONFIG);
		}
	}
	if(g_led_event.handle)
	{
		switch(g_led_event.event_type)
		{
			case LED_CONFIG:
			{
				int res;
				LED_config_range_t LED_config_range = TYPE_FULL;
				(LED_NOT_RESPOND == led_state) ? (LED_config_range = TYPE_9600) : (LED_config_range = TYPE_FULL);
				res = g_led_event.handle((void*)LED_config_range);
				if(res > 0) 
				{
					return;//handle未执行结束 , res==0,ok; res>0, running; res<0, error
				}
				g_led_event.handle = NULL;
				g_ledshowtimer = SysTime();
				timer_gap = 1;
				if(-1 == res)
				{
					led_state = LED_NOT_FOUND;
					g_probetimer = SysTime();
				}
                else if(0 == res)
                {
                    led_state = LED_OK;
                }
				break;
			}
			case LED_SHOW:
			{
				int res;
				res = g_led_event.handle(NULL);
				if(res > 0) return;
				g_led_event.handle = NULL;
			    g_ledshowtimer = SysTime();
				timer_gap = led_panel_params.refresh;
				if(-1 == res)
				{
					led_state = LED_NOT_RESPOND;
					timer_gap = 1;
					g_probetimer = SysTime();
				}
				break;
			}
			default:
				break;
		}
	}
		//else
}


static char yt003_ledcontent_line0[] = "\\FO000\\FE000PM10:999ug/m3            ";
static char yt003_lcformate_line0[]  = "\\FO000\\FE000PM10:%3dug/m3            ";
static char yt003_ledcontent_line1[] = "\\FO000\\FE000温度:-15℃ 湿度:50%RH    ";
static char yt003_lcformate_line1[]  = "\\FO000\\FE000温度:%3d℃ 湿度:%2d%%RH    ";
static char yt003_ledcontent_line2[] = "\\FO000\\FE000风速:11m/s 风向:东北风    ";
static char yt003_lcformate_line2[]  = "\\FO000\\FE000风速:%2dm/s 风向:%s    ";


static char yt004_ledcontent_line0[] = "\\FO000\\FE000PM2.5:999ug/m3 温度:-15℃           ";
static char yt004_lcformate_line0[]  = "\\FO000\\FE000PM2.5:%3dug/m3 温度:%3d℃           ";
static char yt004_ledcontent_line1[] = "\\FO000\\FE000PM10 :999ug/m3 湿度: 50%RH           ";
static char yt004_lcformate_line1[]  = "\\FO000\\FE000PM10 :%3dug/m3 湿度: %2d%%RH           ";
static char yt004_ledcontent_line2[] = "\\FO000\\FE000噪声 : 50dB    东北风 11m/s          ";
static char yt004_lcformate_line2[]  = "\\FO000\\FE000噪声 :%3ddB    %s %2dm/s          ";


static char yt001_ledcontent_line0[] = "\\FO000\\FE000PM25:999 PM10:999 东北风 11m/s     ";
static char yt001_lcformate_line0[]  = "\\FO000\\FE000PM25:%3d PM10:%3d %s %2dm/s    ";
static char yt001_ledcontent_line1[] = "\\FO000\\FE000SO2:1234  CO:2.34 温度:-15℃      ";
static char yt001_lcformate_line1[]  = "\\FO000\\FE000SO2:%4d  CO:%4.2f 温度:%3d℃    ";
static char yt001_ledcontent_line2[] = "\\FO000\\FE000NO2:1234  O3:1234 湿度:50%RH     ";
static char yt001_lcformate_line2[]  = "\\FO000\\FE000NO2:%4d  O3:%4d 湿度:%2d%%RH   ";


static char yt005_ledcontent_line0[] = "\\FO000\\FE000PM25:999 PM10:999 风速:11m/s 东北风       ";
static char yt005_lcformate_line0[]  = "\\FO000\\FE000PM25:%3d PM10:%3d 风速:%2dm/s %s      ";
static char yt005_ledcontent_line1[] = "\\FO000\\FE000SO2:1234 CO:2.34 温度:-15℃ VOC:12.34       ";
static char yt005_lcformate_line1[]  = "\\FO000\\FE000SO2:%4d  CO:%4.2f 温度:%3d℃ VOC:%.2f      ";
static char yt005_ledcontent_line2[] = "\\FO000\\FE000NO2:1234 O3:1234 湿度:50%RH               ";
static char yt005_lcformate_line2[]  = "\\FO000\\FE000NO2:%4d  O3:%4d 湿度:%2d%%RH              ";


static char yt007_ledcontent_line0[] = "\\FO000\\FE000PM2.5:999ug/m3 温度:-15℃           ";
static char yt007_lcformate_line0[]  = "\\FO000\\FE000PM2.5:%3dug/m3 温度:%3d℃           ";
static char yt007_ledcontent_line1[] = "\\FO000\\FE000PM10 :999ug/m3 湿度: 50%RH           ";
static char yt007_lcformate_line1[]  = "\\FO000\\FE000PM10 :%3dug/m3 湿度: %2d%%RH         ";
static char yt007_ledcontent_line2[] = "\\FO000\\FE000风向 :东北风   风速:11m/s          ";
static char yt007_lcformate_line2[]  = "\\FO000\\FE000风向 :%6s   风速:%2dm/s          ";


void led_panel_init()
{    
    srand(10);
    if(strstr(SysGetDevType(),"YT-001"))
    {
        led_line_display_config(yt001, 0, 4, YM_SENSOR_TYPE_PM25, YM_SENSOR_TYPE_PM10, YM_SENSOR_TYPE_WD, YM_SENSOR_TYPE_WS);
        led_line_display_config(yt001, 1, 3, YM_SENSOR_TYPE_SO2, YM_SENSOR_TYPE_CO, YM_SENSOR_TYPE_TP);
        led_line_display_config(yt001, 2, 3, YM_SENSOR_TYPE_NO2, YM_SENSOR_TYPE_O3, YM_SENSOR_TYPE_TD);
        led_panel_params.refresh = 13;
    }
    else if(strstr(SysGetDevType(),"YT-004")||strstr(SysGetDevType(),"YT-014"))
    {
        led_line_display_config(yt004, 0, 2, YM_SENSOR_TYPE_PM25, YM_SENSOR_TYPE_TP);
        led_line_display_config(yt004, 1, 2, YM_SENSOR_TYPE_PM10, YM_SENSOR_TYPE_TD);
        led_line_display_config(yt004, 2, 3, YM_SENSOR_TYPE_NOISE, YM_SENSOR_TYPE_WD, YM_SENSOR_TYPE_WS);
        led_panel_params.refresh = 12;
    }
    else if(strstr(SysGetDevType(),"YT-005"))
    {
        led_line_display_config(yt005, 0, 4, YM_SENSOR_TYPE_PM25, YM_SENSOR_TYPE_PM10, YM_SENSOR_TYPE_WS, YM_SENSOR_TYPE_WD);
        led_line_display_config(yt005, 1, 4, YM_SENSOR_TYPE_SO2, YM_SENSOR_TYPE_CO, YM_SENSOR_TYPE_TP, YM_SENSOR_TYPE_VOC);
        led_line_display_config(yt005, 2, 3, YM_SENSOR_TYPE_NO2, YM_SENSOR_TYPE_O3, YM_SENSOR_TYPE_TD);
        led_panel_params.refresh = 16;
    }  
    else if(strstr(SysGetDevType(),"YT-007")||strstr(SysGetDevType(),"YT-017"))
    {
        led_line_display_config(yt007, 0, 2, YM_SENSOR_TYPE_PM25, YM_SENSOR_TYPE_TP);
        led_line_display_config(yt007, 1, 2, YM_SENSOR_TYPE_PM10, YM_SENSOR_TYPE_TD);
        led_line_display_config(yt007, 2, 2, YM_SENSOR_TYPE_WD, YM_SENSOR_TYPE_WS);
        led_panel_params.refresh = 13;
    }  
    else
    {
        led_line_display_config(yt003, 0, 1, YM_SENSOR_TYPE_PM10);
        led_line_display_config(yt003, 1, 2, YM_SENSOR_TYPE_TP, YM_SENSOR_TYPE_TD);
        led_line_display_config(yt003, 2, 2, YM_SENSOR_TYPE_WS, YM_SENSOR_TYPE_WD);
        led_panel_params.refresh = 12;
    }
    hal_ledpanel_pin_config();
    set_ledpanel_byte_recv_handler(ledpanel_byte_recv_handler);
}
#else

void led_panel_poll(void)
{

}
void led_panel_init(void)
{

}

#endif





