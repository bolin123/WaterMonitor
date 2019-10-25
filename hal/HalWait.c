#include "HalWait.h"
#include "stm32f10x_tim.h"

static int volatile g_count;

void HalWaitMsDone(void)
{
    TIM_ClearITPendingBit(TIM6, TIM_FLAG_Update);
    TIM_Cmd(TIM6, DISABLE);
}

void HalWaitMsInit(uint32_t ms)
{    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

    g_count = ms;
    TIM_DeInit(TIM6);
    TIM_TimeBaseInitStruct.TIM_Prescaler = 1000;
    TIM_TimeBaseInitStruct.TIM_Period = 72 - 1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseInitStruct);

    TIM_ClearITPendingBit(TIM6, TIM_FLAG_Update);
    TIM_Cmd(TIM6, ENABLE);
}

bool HalWaitMsTimeup(void)
{
    if(TIM_GetFlagStatus(TIM6, TIM_FLAG_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM6, TIM_FLAG_Update);
        g_count--;
        if(g_count <= 0)
        {
            return true;
        }
    }
    
    return false;
}


