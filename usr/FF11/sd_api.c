#include "sd_api.h"
#include "stdlib.h"
#include "string.h"
#include "ff.h"
#include "Sys.h"
#include "stm32f10x.h"
#include "stm32f10x_exti.h"


static uint8_t SDPEnable = 0;
#define SDPrintf(...) do\
							{\
							    if(SDPEnable)\
							      SysPrintf(__VA_ARGS__);\
							}while(0)
							
FATFS fs;
FIL fdst;
static uint8_t is_sd_detected = 1;

#define BIG_FILE_BYTES  100*1024*1024UL//1024*1024
#define FILE_SUM_MAX	10//5

static void bigfile_rename(char *path);
static uint16_t filesum_limit(char *format);

static int fatfs_sd_write(char *path,char *temp,uint16_t len);
//static int fatfs_sd_read(char *path,char *temp,uint16_t len,DWORD offset);



static void bigfile_rename(char *oldpath)
{
	FRESULT res;
	u8 oldpath_len,newpath_len;
	char *newpath = NULL;
	oldpath_len = strlen(oldpath);
	newpath_len = oldpath_len+6;
	newpath = (char*)SysMalloc(newpath_len+1);
	if(newpath == NULL)
	{
		SDPrintf("%s malloc failed\n",__func__);
		return;
	}
	memset(newpath,0,newpath_len+1);
	strncpy(newpath,oldpath,oldpath_len-4);//去掉.TXT
	char suffix[6+4+1];
	struct YMDateTime_st *time = SysGetDateTime();
	sprintf(suffix,"%02d%02d%02d.TXT",time->year > 2000 ? (time->year - 2000) : 0,time->month,time->day);
    //sprintf(suffix,"%02d%02d%02d.TXT",time->month,time->day,time->hour);
    strcat(newpath,suffix);
	SDPrintf("oldpath:%s\n",oldpath);
	SDPrintf("newpath:%s\n",newpath);
	res = f_rename(oldpath,newpath);
	if(res == FR_OK)
	{
		SDPrintf("%s succes\n",__func__);
	}
	else
	{
		SDPrintf("%s failed,%d\n",__func__,res);
        goto error;
	}
	if(strstr(oldpath,"AD"))
	{
		filesum_limit("AD*.TXT");
	}
	else if(strstr(oldpath,"RD"))
	{
		filesum_limit("RD*.TXT");
	}
	free(newpath);
error:
    if(res != FR_OK)
    {
        fs.fs_type =  0;
    }
    return;
}

static int fatfs_sd_write(char *path,char *temp,uint16_t len)
{
    int result = 0;
    FRESULT res;
	UINT bw;            // File R/W count
	if (0 == fs.fs_type) 
	{					/* If the volume has NOT been mounted */
		SDPrintf("start f_mount\n");
		res = f_mount(&fs,"1:",1);	 
        SDPrintf("f_mount res = %d\n",res);
		if(res != FR_OK)
		{
		    is_sd_detected = 0;
			//add error_handler
            YMFaultsNumSet(HAL_ERROR_CODE_SDCARD, true);
			return -1;
		}
        YMFaultsNumSet(HAL_ERROR_CODE_SDCARD, false);
	}
    if(!is_sd_detected)
        return -1;
	//res = f_open(&fdst,path,FA_CREATE_NEW | FA_WRITE); SDPrintf("f_open res = %d\n",res);
	res = f_open(&fdst,path,FA_OPEN_ALWAYS|FA_WRITE); 
    SDPrintf("f_open res  = %d\n",res);

  	if (res == FR_OK)
 	{
		DWORD file_size = 0;

		file_size =  f_size(&fdst);                       
        SDPrintf("file_size   = %lu B\n",(unsigned long)file_size);
		res = f_lseek(&fdst, file_size);                  
        SDPrintf("f_lseek res = %d\n",res);
		if(res != FR_OK)
            goto error;
	    res = f_write(&fdst, temp, len, &bw);             
        SDPrintf("f_write res = %d\n", res);//117 = 13*9, 135 = 15*9
		if(res != FR_OK)
            goto error;
        if(*(temp+len-1) != '\n')
            f_write(&fdst, "\r\n", 2, &bw); 
		res = f_close(&fdst);
		if(res != FR_OK)
            goto ok;
		SDPrintf("sd rw over\n");
		if(file_size > BIG_FILE_BYTES)
		{
			SDPrintf("in bigfile_rename\n");
			bigfile_rename(path);
			SDPrintf("out bigfile_rename\n");
		}
		goto ok;
 	}
	else
	{
		SDPrintf("f_open failed,%d\n",res);
	}
	//计算SD卡使用情况
//	f_getfree("1:",&fre_clust,&fs_p); 
//	SD_capacity.usage = 100.0-(float)fre_clust/(fs.n_fatent-2.0)*100.0;
//	SDPrintf("the SD capacity usage is %f\n",SD_capacity.usage);
	
	//f_mount(NULL,"1:",0);
error:
    result = -1;
    if(res != FR_OK)
    {
        fs.fs_type =  0;
    }
	return result;
ok://fclose返回错误时，其实写入是成功的
    result = 0;
    if(res != FR_OK)
    {
        fs.fs_type =  0;
    }
	return result;
}
/*
static int fatfs_sd_read(char *path,char *temp,u16 len,DWORD offset)
{
	FRESULT res;
	UINT bw;            // File R/W count

//	FATFS *fs_p = &fs;
 
	res = f_mount(&fs,"1:",1);		
	res = f_open(&fdst,path,FA_OPEN_EXISTING|FA_READ);	 
 	if(res == FR_OK)
	{
		DWORD file_size;
		file_size =  f_size(&fdst);
		res = f_lseek(&fdst, file_size - offset);
	    res = f_read(&fdst, temp, len, &bw);  
		f_close(&fdst);
 	}	
	else
	{
		f_close(&fdst);
	}
	f_mount(NULL,"1:",0);
	return 0;
}
*/
static uint16_t filesum_limit(char *format)
{
	FRESULT res;
	DIR dj;
	FILINFO fno;
	//FATFS *fs_p = &fs;
	uint16_t cnt = 0;
	char oldest_fname[13] = {0};
	if (0 == fs.fs_type) 
	{					/* If the volume has NOT been mounted */
		SDPrintf("start f_mount\n");
		res = f_mount(&fs,"1:",1);	 
        SDPrintf("f_mount res = %d\n",res);
		if(res != FR_OK)
		{
		    is_sd_detected = 0;
			//add error_handler 
			return 0;
		}
	}

	res = f_findfirst(&dj,&fno,"1:",format);
    if(res != FR_OK)
        goto error;
	while(res == FR_OK && fno.fname[0])
	{
		SDPrintf("%s\n",fno.fname);
		if(strlen(fno.fname) == 12)
		{
			cnt++;
			if((0 == oldest_fname[0])||(strcmp(fno.fname,oldest_fname) < 0))
			{
				strcpy(oldest_fname,fno.fname);
			}				
		}
		res = f_findnext(&dj,&fno);
        if(res != FR_OK)
            goto error;
	}
	SDPrintf("oldest_fname: %s\n",oldest_fname);
	if(cnt > FILE_SUM_MAX)
	{
		char oldest_fname_path[16] = {0};
		strcat(oldest_fname_path,"1:/");
		strcat(oldest_fname_path,oldest_fname);
		res = f_unlink(oldest_fname_path);
		SDPrintf("unlink %s ,res = %d\n",oldest_fname_path,res);
        if(res != FR_OK)
            goto error;
	}
	res = f_closedir(&dj);
    if(res != FR_OK)
        goto error;
	return cnt;
error:
    fs.fs_type =  0;
	return 0;
}

void sd_record(char *path, float array[], uint8_t num)
{
	char *buffer = (char*)SysMalloc(200);
	if(NULL == buffer)
		return;
	memset(buffer,0,200);
	struct YMDateTime_st *time = SysGetDateTime();
	sprintf(buffer,"%4d-%02d-%02dT%02d:%02d:%02d,",time->year,time->month,time->day,time->hour,time->min,time->sec);
	for(uint8_t i = 0; i < num; i++)
	{
		sprintf(buffer + strlen(buffer),"%8.3f,",array[i]);
	}
	strcat(buffer,"\n");
	
	fatfs_sd_write(path,buffer,strlen(buffer));
	free(buffer);
}

void sd_detectPin_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE );	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;   //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	EXTI_InitTypeDef EXTI_InitStructure;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC,GPIO_PinSource7);

	EXTI_InitStructure.EXTI_Line=EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Rising_Falling;  
	EXTI_InitStructure.EXTI_LineCmd=ENABLE; 
	EXTI_Init (&EXTI_InitStructure) ; 
}

void sd_detectPin_change_cb(void)
{
	if(GPIOC->IDR & GPIO_Pin_7)
	{
		fs.fs_type = 0;
	}
	else
	{
		fs.fs_type = 0;
		is_sd_detected = 1;//__NOP();
	}
}

/*
precondition:

function:
  通过f_mount操作判断SD卡是否出错
return:
  0: 正常
 -1: sd卡硬件错误，或sd卡不存在
*/
int8_t IsSDHalError()
{
    FRESULT res;
    if (0 == fs.fs_type) 
	{					/* If the volume has NOT been mounted */
		SDPrintf("start f_mount\n");
		res = f_mount(&fs,"1:",1);	 
        SDPrintf("f_mount res = %d\n",res);
		if(res != FR_OK)
		{
		    is_sd_detected = 0;
			//add error_handler
			YMFaultsNumSet(HAL_ERROR_CODE_SDCARD, true);
			return 1;
		}
        YMFaultsNumSet(HAL_ERROR_CODE_SDCARD, false);
	}
    if(!is_sd_detected)
        return 1;
    return 0;
}

static int lastMessagePostion(void)
{
    char byte;
    UINT rdNum;
    int rdPos = fdst.fsize - 2;
    
    while(1)
    {
        f_lseek(&fdst, rdPos); //ignor "\r\n"
        f_read(&fdst, &byte, 1, &rdNum);
        if(rdNum != 1)
        {
            return -1;
        }
        if(byte == '\n')
        {
            return (rdPos + 1);
        }
        rdPos--;

        if(rdPos == 0)
        {
            return 0;
        }
    }
}

static const char *filePathName(uint8_t id)
{
    static char path[16] = "";

    sprintf(path, "1:/LOST%d.TXT", id);
    return path;
}

static int8_t g_msgNum[2] = {0, 0};

static int getMessageNum(uint8_t id)
{
    DWORD i;
    int ret = 0;
    UINT rdNum;
    char byte;
    int count = 0;

    if(IsSDHalError())
    {
        SysLog("L:%d", __LINE__);
        return -1;
    }

    
    if(f_open(&fdst, filePathName(id), FA_OPEN_ALWAYS|FA_WRITE|FA_READ) != FR_OK)
    {
        SysLog("File %s open error!", filePathName(id));
        ret = -1;
        goto error_handle;
    }

    for(i = 0; i < fdst.fsize; i++)
    {
        f_read(&fdst, &byte, 1, &rdNum);
        if(rdNum != 1)
        {
            ret = -2;
            goto error_handle;
        }
        if(byte == '\n')
        {
            count++;
        }
    }

    ret = count;
    
error_handle:
    if(f_close(&fdst) != FR_OK)
    {
        fs.fs_type =  0;
    }
    return ret;

}

void HalFileUpdateInfo(void)
{   
    uint8_t i;

    for(i = 0; i < sizeof(g_msgNum); i++)
    {
        g_msgNum[i] = getMessageNum(i);
        
        if(g_msgNum[i] < 0)
        {
            g_msgNum[i] = 0;
        }
    }
}

int HalFileGetNextMessage(uint8_t id, char *buff, uint16_t length)
{
    int ret = 0, i;
    UINT rdptr;
    int rdPos;

    if(g_msgNum[id] == 0)
    {
        return -2;
    }
    
    if(IsSDHalError())
    {
        SysLog("L:%d", __LINE__);
        return -1;
    }

    if(f_open(&fdst, filePathName(id), FA_OPEN_ALWAYS|FA_WRITE|FA_READ) != FR_OK)
    {
        //SysLog("File %s open error!", filePathName(id));
        ret = -1;
        goto error_handle;
    }

    if(fdst.fsize <= 2) //空文件
    {
        ret = -2;
        goto error_handle;
    }

    rdPos = lastMessagePostion(); //查找最后一条消息
    if(rdPos < 0)
    {
        ret = -2;
        goto error_handle;
    }
    f_lseek(&fdst, rdPos);
    
    for(i = 0; i < length; i++)
    {
        f_read(&fdst, &buff[i], 1, &rdptr);
        if(rdptr != 1)
        {
            //SysLog("L:%d", __LINE__);
            ret = -1;
            goto error_handle;
        }

        if(buff[i] == '\r')
        {
            buff[i] = '\0';
            break;
        }
    }

    if(i >= length)
    {
        //SysLog("L:%d", __LINE__);
        ret = -2;
        goto error_handle;
    }

    ret = i;

error_handle:
    if(f_close(&fdst) != FR_OK)
    {
        fs.fs_type =  0;
    }
    return ret;
}

int HalFileDelLastMessage(uint8_t id)
{
    int ret = 0;
    int rdPos;
    
    if(f_open(&fdst, filePathName(id), FA_OPEN_ALWAYS|FA_WRITE|FA_READ) != FR_OK)
    {
        //SysLog("File %s open error!", filePathName(id));
        ret = -1;
        goto error_handle;
    }
    
    rdPos = lastMessagePostion(); //查找最后一条消息
    if(rdPos < 0)
    {
        ret = -2;
        goto error_handle;
    }
    f_lseek(&fdst, rdPos);
    
    if(f_truncate(&fdst) != FR_OK)
    {
        ret = -1;
        goto error_handle;
    }
    
    g_msgNum[id]--;
error_handle:
    if(f_close(&fdst) != FR_OK)
    {
        fs.fs_type =  0;
    }
    return ret;
}

int HalFileInsertMessage(uint8_t id, const char* msg)
{
    int ret = 0;
    UINT wrNum;
    
    if(g_msgNum[id] >= SYS_SAVE_LOST_MSG_NUM_MAX)
    {
        return -2;
    }
    
    if(IsSDHalError())
    {
        //SysLog("L:%d", __LINE__);
        return -1;
    }
    
    if(f_open(&fdst, filePathName(id), FA_OPEN_ALWAYS|FA_WRITE|FA_READ) != FR_OK)
    {
        //SysLog("File %s open error!", filePathName(id));
        ret = -1;
        goto error_handle;
    }

    f_lseek(&fdst, fdst.fsize);

    f_write(&fdst, msg, strlen(msg), &wrNum); //messsage
    if(wrNum != strlen(msg))
    {
        ret = -1;
        goto error_handle;
    }
    
    f_write(&fdst, "\r\n", 2, &wrNum); //end
    if(wrNum != 2)
    {
        ret = -1;
        goto error_handle;
    }
    g_msgNum[id]++;
error_handle:
    if(f_close(&fdst) != FR_OK)
    {
        fs.fs_type =  0;
    }
    return ret;
}

int HalFileRecord(char *data)
{
    return fatfs_sd_write(AVERAGE_DATA_FILE_PATH, data, strlen(data));
}



