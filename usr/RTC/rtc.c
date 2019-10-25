#include "rtc.h" 
#include "Sys.h"
#include "delay.h"
#include "stm32f10x_bkp.h"
#include "stm32f10x_pwr.h"
#include "stm32f10x_rtc.h"
#include "stdio.h"
#include "PowerManager.h"
#include "HalWait.h"

static bool g_initok = false;

static void RTC_NVIC_Config(void)
{	
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;		//RTC全局中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//先占优先级1位,从优先级3位
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	//先占优先级0位,从优先级4位
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//使能该通道中断
    NVIC_Init(&NVIC_InitStructure);		//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}

bool rtc_init_success(void)
{
    return g_initok;
}

//实时时钟配置
//初始化RTC时钟,同时检测时钟是否工作正常
//BKP->DR1用于保存是否第一次配置的设置
//返回0:正常
//其他:错误代码

uint8_t rtc_init(void)
{
	//检查是不是第一次配置时钟
//	uint8_t temp=0;
 
	if (BKP_ReadBackupRegister(BKP_DR1) != 0x5050)		//从指定的后备寄存器中读出数据:读出了与写入的指定数据不相乎
	{	 			
	    SysLog("rtc ch1 init ...");
    	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟   
    	PWR_BackupAccessCmd(ENABLE);	//使能后备寄存器访问 
    	BKP_DeInit();	//复位备份区域 	
    	RCC_LSEConfig(RCC_LSE_ON);	//设置外部低速晶振(LSE),使用外设低速晶振
    	HalWaitCondition((RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET), 6000);
    	if(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) //是否成功
        {
            SysLog("ch1 init failed!");
            YMFaultsNumSet(HAL_ERROR_CODE_RTC, true);
            return 1;
        }   
    	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//设置RTC时钟(RTCCLK),选择LSE作为RTC时钟    
    	RCC_RTCCLKCmd(ENABLE);	//使能RTC时钟  
    	RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
    	RTC_WaitForSynchro();		//等待RTC寄存器同步  
    	//RTC_ITConfig(RTC_IT_SEC, ENABLE);		//使能RTC秒中断
    	//RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
    	RTC_EnterConfigMode();/// 允许配置	
    	RTC_SetPrescaler(32767); //设置RTC预分频的值
    	RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成

    	RTC_ExitConfigMode(); //退出配置模式  
    	BKP_WriteBackupRegister(BKP_DR1, 0X5050);	//向指定的后备寄存器中写入用户程序数据
	}
	else//系统继续计时
	{
	    SysLog("rtc ch2 init ...");
    	//RTC_WaitForSynchro();	//等待最近一次对RTC寄存器的写操作完成
        RTC->CRL &= (uint16_t)~RTC_FLAG_RSF;
        // Loop until RSF flag is set 
        HalWaitCondition(((RTC->CRL & RTC_FLAG_RSF) == (uint16_t)RESET), 6000);
    	if((RTC->CRL & RTC_FLAG_RSF) == (uint16_t)RESET) //是否成功
        {
            SysLog("ch2 init failed!");
            YMFaultsNumSet(HAL_ERROR_CODE_RTC, true);
            return 1;
        }   
        SysSetDateTimeFromRtc();
	}
	RTC_NVIC_Config();//RCT中断分组设置		    				     
    SysLog("rtc init done");
    g_initok = true;
	return 0; //ok

}	
static bool IT_alr;
void rtc_set_flag_IT_alr()
{
    IT_alr = true;
}
void rtc_clear_flag_IT_alr()
{
    IT_alr = false;
}
bool rtc_get_flag_IT_alr()
{
    return IT_alr;
}

void RTC_IRQHandler(void)
{		 
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)//秒钟中断
	{							
		RTC_ClearITPendingBit(RTC_IT_SEC);
 	}
	if(RTC_GetITStatus(RTC_IT_ALR)!= RESET)//闹钟中断
	{
		RTC_ClearITPendingBit(RTC_IT_ALR);		//清闹钟中断	  
        SysRtcTimeup(true);
        rtc_set_flag_IT_alr();
  	} 				  								 
	RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//清闹钟中断
	RTC_WaitForLastTask();	  	    						 	   	 
}

uint32_t rtc_get_counter(void) 
{
	return RTC_GetCounter();
}

uint32_t g_alarm_cntx;
void rtc_alarm_set(uint32_t cntx)
{
    g_alarm_cntx = cntx;
    RCC->APB1ENR|=1<<28;
    RCC->APB1ENR|=1<<27;
    PWR->CR|=1<<8;   

    RTC->CRL|=1<<4;   
    RTC->CRH|=1<<1;  
    RTC->ALRL=cntx&0xffff;
    RTC->ALRH=cntx>>16;
    RTC->CRL&=~(1<<4);
    HalWaitCondition((!(RTC->CRL&(1<<5))), 6000);
}  

uint32_t rtc_alarm_get(void)
{
    return g_alarm_cntx;
}

static uint8_t is_leap_year(uint16_t year)
{			  
	if(year%4==0)
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1; 
			else return 0;   
		}else return 1;   
	}else return 0;	
}	 			   
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};										 
uint8_t rtc_set(struct YMDateTime_st sysDataTime)
{
	uint16_t t;
	uint32_t seccount=0;
	if(sysDataTime.year<1970||sysDataTime.year>2099)return 1;	   
	for(t=1970;t<sysDataTime.year;t++)
	{
		if(is_leap_year(t))seccount+=31622400;
		else seccount+=31536000;
	}
	sysDataTime.month-=1;
	for(t=0;t<sysDataTime.month;t++)	  
	{
		seccount+=(uint32_t)mon_table[t]*86400;
		if(is_leap_year(sysDataTime.year)&&t==1)seccount+=86400;
	}
	seccount+=(uint32_t)(sysDataTime.day-1)*86400;
	seccount+=(uint32_t)sysDataTime.hour*3600;
    seccount+=(uint32_t)sysDataTime.min*60;	 
	seccount+=sysDataTime.sec;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	  
	PWR_BackupAccessCmd(ENABLE);	
	RTC_SetCounter(seccount);

	HalWaitCondition(((RTC->CRL & RTC_FLAG_RTOFF) == (uint16_t)RESET), 2000);
	return 0;	    
}

uint32_t rtc_utc_value(void)
{
    return RTC_GetCounter();
}

struct YMDateTime_st rtc_get(void)
{
    struct YMDateTime_st sysDataTime;
	uint32_t timecount=0; 
	uint32_t temp=0;
	uint16_t temp1=0;	  
    timecount=RTC_GetCounter();
 	temp=timecount/86400; 

	temp1=1970;	
	while(temp>=365)
	{				 
		if(is_leap_year(temp1))
		{
			if(temp>=366)temp-=366;
			else if(temp == 365) break;
			else {temp1++;break;}  
		}
		else temp-=365;	 
		temp1++;  
	}   
	sysDataTime.year=temp1;
	temp1=0;
	while(temp>=28)
	{
		if(is_leap_year(sysDataTime.year)&&temp1==1)
		{
			if(temp>=29)temp-=29;
			else break; 
		}
		else 
		{
			if(temp>=mon_table[temp1])temp-=mon_table[temp1];
			else break;
		}
		temp1++;  
	}
	sysDataTime.month=temp1+1;
	sysDataTime.day=temp+1;

	temp=timecount%86400;     		   
	sysDataTime.hour=temp/3600;     
	sysDataTime.min=(temp%3600)/60; 
	sysDataTime.sec=(temp%3600)%60;   
	return sysDataTime;
}	 



