#include "HalWatchdog.h"
#include "Sys.h"
#include "stm32f10x_tim.h"

static volatile uint8_t g_wdgActionCount = 0;

void HalWatchdogFeed(void)
{
    TIM_Cmd(TIM2, DISABLE);
    g_wdgActionCount = 0;
    TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void)
{
    if(TIM_GetFlagStatus(TIM2, TIM_FLAG_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
        g_wdgActionCount++;
        if(g_wdgActionCount > 3) //8S?¡ä??1¡¤
        {
            SysPrintf("!!!!!!!watch dog action!\n");
            SysReboot();
        }
    }
}

void HalWatchdogDisable(void)
{
    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
}

void HalWatchdogInitialize(void)
{   
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

    TIM_TimeBaseInitStruct.TIM_Prescaler = 20000 - 1;
    TIM_TimeBaseInitStruct.TIM_Period = 7200 - 1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ClearITPendingBit(TIM2, TIM_FLAG_Update);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}

