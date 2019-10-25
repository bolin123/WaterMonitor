#ifndef __RTC_H
#define __RTC_H	 

#include "Sys.h"
#include "stm32f10x.h"
#include "stm32f10x_pwr.h"

uint8_t rtc_init(void);        //初始化RTC,返回0,失败;1,成功;
bool rtc_init_success(void);
struct YMDateTime_st rtc_get(void);
uint8_t rtc_set(struct YMDateTime_st sysDataTime);
uint32_t rtc_get_counter(void) ;
uint32_t rtc_utc_value(void);
void rtc_alarm_set(uint32_t cntx);
uint32_t rtc_alarm_get(void);
void rtc_set_flag_IT_alr(void);
void rtc_clear_flag_IT_alr(void);
bool rtc_get_flag_IT_alr(void);

#endif


