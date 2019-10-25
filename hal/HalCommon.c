#include "HalCommon.h"
#include "Sys.h"

static volatile uint32_t g_sysTimeCount = 0;
static uint8_t g_blinkCount = 0;
static bool g_isRtcWakeup = false;

//redirect "printf()"
int fputc(int ch, FILE *f)
{
#if defined(HAL_BOARD_NAME_BASIC)
    HalUartWrite(HAL_UART_LOG_PORT, (const uint8_t *)&ch, 1);
#elif defined(HAL_BOARD_NAME_LITE)
    HalUartWrite(HAL_UART_LOG_PORT, (const uint8_t *)&ch, 1);
#else
    //HalUartWrite(HAL_UART_LOG_PORT, (const uint8_t *)&ch, 1);
#endif
    return ch;
}

void HardFault_Handler(void)
{
    uint8_t i;

    for(i = 0; i < 10; i++)
    {
        SysLog("");
    }
    HalCommonReboot();
}

static void statusLedBlink(void)
{
    static uint8_t blinkCount = 0;
    static SysTime_t lastBlinkTime = 0;

    if(blinkCount < g_blinkCount)
    {
        if(SysTimeHasPast(lastBlinkTime, 200))
        {
            HalGPIOSetLevel(HAL_STATUS_LED_PIN, !HalGPIOGetLevel(HAL_STATUS_LED_PIN));
            lastBlinkTime = SysTime();
            blinkCount++;
        }
    }
    else
    {
        HalGPIOSetLevel(HAL_STATUS_LED_PIN, HAL_STATUS_LED_DISABLE_LEVEL);
        if(SysTimeHasPast(lastBlinkTime, 1500))
        {
            //lastBlinkTime = SysTime();
            blinkCount = 0;
        }
    }
}

static void statusLedInit(void)
{
    HalGPIOConfig(HAL_STATUS_LED_PIN, HAL_IO_OUTPUT);
    HalGPIOSetLevel(HAL_STATUS_LED_PIN, HAL_STATUS_LED_ENABLE_LEVEL);
}

void HalCommonStatusLedSet(uint8_t blink)
{
    g_blinkCount = blink * 2;
}

extern void APPDiDaTick(void);
void HalTimerPast1ms(void)
{
    g_sysTimeCount++;
    APPDiDaTick();
}

uint32_t HalGetSysTimeCount(void)
{
    return g_sysTimeCount;
}

void HalInterruptSet(bool enable)
{
    if(enable)
    {
        __enable_irq();
    }
    else
    {
        __disable_irq();
    }
}

static void periphClockInit(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
}

static void getWakeupMode(void)
{
    if(RCC_GetFlagStatus(RCC_FLAG_PINRST))
    {
        g_isRtcWakeup = false;
    }
    else
    {
        g_isRtcWakeup = true;
    }
    RCC_ClearFlag();
}

HalBootMode_t HalCommonBootMode(void)
{
    if(g_isRtcWakeup)
    {
        return HAL_BOOT_MODE_RTC;
    }

    return HAL_BOOT_MODE_NORMAL;
}

void HalCommonReboot(void)
{
    __set_FAULTMASK(1); //ÖÕÖ¹ËùÓÐÖÐ¶Ï
    NVIC_SystemReset();
}

void HalCommonResume(void)
{
    HalGPIOSetLevel(HAL_DB_POWER_PIN, HAL_DB_POWER_ON_LEVEL);
    HalTimerInitialize();
    HalWatchdogInitialize();
}

void HalCommonSleep(void)
{
    SysLog("entry");
    HalGPIOSetLevel(HAL_DB_POWER_PIN, HAL_DB_POWER_OFF_LEVEL);
    HalTimerDisable();
    HalWatchdogDisable();

    PWR->CR |= PWR_CR_CWUF;
    #if defined ( __CC_ARM )
    __force_stores();
    #endif
    __WFI();

    SysLog("exit");
}

void HalCommonStandBy(void)
{
    HalGPIOSetLevel(HAL_DB_POWER_PIN, HAL_DB_POWER_OFF_LEVEL);

    /* Clear Wake-up flag */
    PWR->CR |= PWR_CR_CWUF;
    /* Select STANDBY mode */
    PWR->CR |= PWR_CR_PDDS;
    /* Set SLEEPDEEP bit of Cortex System Control Register */
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    /* This option is used to ensure that store operations are completed */
    #if defined ( __CC_ARM   )
    __force_stores();
    #endif
    /* Request Wait For Interrupt */
    __WFI();
}

extern void SysCmdByteRecv(uint8_t *data, uint16_t len);

static void logPrintInit(void)
{
#if defined(HAL_BOARD_NAME_BASIC)
    HalUartConfig_t config;
    config.baudrate = 115200;
    config.flowControl = 0;
    config.parity = 0;
    config.wordLength = USART_WordLength_8b;
    config.recvCb = SysCmdByteRecv;
    HalUartConfig(HAL_UART_LOG_PORT, &config);
#elif defined(HAL_BOARD_NAME_LITE)
    HalUartConfig_t config;
    config.baudrate = 115200;
    config.flowControl = 0;
    config.parity = 0;
    config.wordLength = USART_WordLength_8b;
    config.recvCb = SysCmdByteRecv;
    HalUartConfig(HAL_UART_LOG_PORT, &config);

#else
#endif
}

static void powerCtrlIOInit(void)
{
    HalGPIOConfig(HAL_DB_POWER_PIN, HAL_IO_OUTPUT);
    HalGPIOSetLevel(HAL_DB_POWER_PIN, HAL_DB_POWER_ON_LEVEL);
}

void HalCommonInitialize(void)
{
    getWakeupMode();
    SystemInit();
    periphClockInit();
    HalGPIOInitialize();
    HalFlashInitialize();
    HalUartInitialize();
    HalTimerInitialize();
	HalADCInitialize();
    powerCtrlIOInit();
    statusLedInit();
    logPrintInit();
}

void HalCommonPoll(void)
{
    HalUartPoll();
    HalGPIOPoll();
    HalTimerPoll();
    HalFlashPoll();
    HalWatchdogFeed();
    statusLedBlink();
}

