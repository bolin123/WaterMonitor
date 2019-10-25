#ifndef HAL_WAIT_H
#define HAL_WAIT_H

#include "HalCtype.h"

void HalWaitMsInit(uint32_t ms);
bool HalWaitMsTimeup(void);
void HalWaitMsDone(void);

#define HalWaitCondition(condition, timeout) \
                                             do{\
                                                 HalWaitMsInit(timeout); \
                                                 while(condition)\
                                                 {\
                                                     if(HalWaitMsTimeup())\
                                                     {\
                                                         break;\
                                                     }\
                                                 }\
                                                 HalWaitMsDone();\
                                             }while(0)

#endif

