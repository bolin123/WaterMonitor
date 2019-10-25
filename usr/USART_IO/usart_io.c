#include "usart_io.h"
#include "delay.h"
#include "stdio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_tim.h"
#include "HalUart.h"
#include "sys.h"

#define SendingDelay 7

static void UartIO_GPIO_init(void);
static void simUart_Send_Byte(u8 Data);
static void TIME3_init(u16 arr,u16 psc);
static HalUartDataRecv_cb g_UartIO_recv_handler;

void set_UartIO_recv_handler(HalUartDataRecv_cb f)
{
	g_UartIO_recv_handler = f;
}

/*********************************
 * 9600  	TIM3_init(108,72)
 * 115200 TIM3_init(10,72)
 * 此处设置波特率9600
********************************/
void SimUart_send_str(char *str);
void SimulateUartConfig(uint8_t uart, HalUartConfig_t *config)
{
    if(config->baudrate == 115200)
    {
        UartIO_GPIO_init();
    	TIME3_init(9, 72);//HCLK 72M
    	set_UartIO_recv_handler(config->recvCb);
    }
    else
    {
        SysPrintf("uart io baudrate must be 9600!\n");
    }
}

static void EXTI9_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOE|RCC_APB2Periph_AFIO, ENABLE );	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;   //上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOE,GPIO_PinSource9);

	EXTI_InitStructure.EXTI_Line=EXTI_Line9;
	EXTI_InitStructure.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger=EXTI_Trigger_Falling;  
	EXTI_InitStructure.EXTI_LineCmd=ENABLE; 
	EXTI_Init (&EXTI_InitStructure) ; 
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;  
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;  
	NVIC_Init(&NVIC_InitStructure); 
}
static void UartIO_send_pin_config()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOE|RCC_APB2Periph_AFIO, ENABLE );	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);	
	GPIO_SetBits(GPIOE,GPIO_Pin_8);//拉高，不然发送时第一位有问题
}
static void UartIO_recv_pin_config()
{
	EXTI9_config();
}

void UartIO_GPIO_init()
{
	UartIO_send_pin_config();//PE8
	UartIO_recv_pin_config();//PE9
}
void simUart_Send_Byte(u8 Data)		// PE8
{
	u8 i=8;
    __disable_irq() ;
	GPIO_ResetBits(GPIOE,GPIO_Pin_8);	
	delay_us(SendingDelay);
	while(i--)  
	{
		if(Data&0x01)
			GPIO_SetBits(GPIOE,GPIO_Pin_8);
		else
			GPIO_ResetBits(GPIOE,GPIO_Pin_8);
		delay_us(SendingDelay);
		Data = Data>>1;
	}
	GPIO_SetBits(GPIOE,GPIO_Pin_8);
	delay_us(SendingDelay);
    __enable_irq() ;
}

void SimUart_send_str(char *str)
{
	while(*str)
		simUart_Send_Byte(*str++); 
}

void SimUart_send_buf(u8 *_ucaBuf, u16 _usLen)
{
	u8 i=0;
	while(i++ <_usLen)
		simUart_Send_Byte(*_ucaBuf++); 
}

static u8 i_Byte = 0;
void EXTI9_5_IRQHandler(void)
{
	if(EXTI_GetFlagStatus(EXTI_Line9) != RESET)
	{		
		i_Byte = 0;
		EXTI->IMR &= ~(1<<9);   			//屏蔽外部中断
		TIM_SetCounter(TIM3, 0);    //设置计数机寄存器值
		TIM_Cmd(TIM3,ENABLE); 
		EXTI_ClearITPendingBit(EXTI_Line9);
	}
	if(EXTI_GetFlagStatus(EXTI_Line7) != RESET)
	{
		extern void sd_detectPin_change_cb(void);
		sd_detectPin_change_cb();
		EXTI_ClearITPendingBit(EXTI_Line7);
	}
}

        

void TIME3_init(u16 arr,u16 psc)
{

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
     
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = arr -1;
    TIM_TimeBaseStructure.TIM_Prescaler = psc-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;      
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_ClearITPendingBit(TIM3, TIM_FLAG_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); 
    TIM_Cmd(TIM3, DISABLE);	
}

uint8_t DATA;  
void TIM3_IRQHandler(void)
{  
	u8 tmp;
	if(TIM_GetFlagStatus(TIM3, TIM_FLAG_Update) != RESET)
	{
		tmp = GPIO_ReadInputDataBit(GPIOE, GPIO_Pin_9);
		if(tmp == 1)
			DATA |= (1 << i_Byte); 
		else
			DATA &= ~(1 << i_Byte);
		i_Byte++;
		if(i_Byte >= 8)
		{
			i_Byte = 0;
			g_UartIO_recv_handler(&DATA, 1);
			EXTI->IMR |= 1<<9;   //使能外部中断
			TIM_Cmd(TIM3,DISABLE); //关闭TIM14
		}
		TIM_ClearITPendingBit(TIM3, TIM_FLAG_Update);
	}
}


void IO_UART_TEST()
{
	while(1)
	{
//		IO_send_str("Hello World\n");
		delay_ms(1000);
	}
}

void Check_IRQ_Error()
{
	if(!(EXTI->IMR & (1<<9)) && !(TIM3->CR1 & TIM_CR1_CEN)) //出现异常，定时器和外部中断都没有使能，串口数据无法获取
	{
		EXTI->IMR |= 1<<9;   //使能外部中断
	}
}





