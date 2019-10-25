#include "ledpanel_hardware.h"
#include "VTStaticQueue.h"
#include "stdio.h"
#include "HalUart.h"
#include "HalGPIO.h"

#define LEDPANEL_CACHE_LEN	128
static VTSQueueDef(unsigned char, g_ledpanelRecvQueue, LEDPANEL_CACHE_LEN);
static byte_recv_handler g_byte_recv_handler = NULL;
static void uart3_recv_cb(u8 *data,u16 len);


void hal_ledpanel_com_config(uint16_t baud)
{
    HalUartConfig_t config;
    config.baudrate = baud;
    config.flowControl = 0;
    config.parity = 0;
    config.wordLength = USART_WordLength_8b;
    config.recvCb = uart3_recv_cb;
    HalUartConfig(HAL_UART_DISPLAY_PORT, &config);
}

void hal_ledpanel_send_enable(uint8_t enable)
{
#if 0
	if(enable)
		GPIO_SetBits(GPIOE,GPIO_Pin_13);
	else
		GPIO_ResetBits(GPIOE,GPIO_Pin_13);
#endif
    if(enable)
    {
        HalGPIOSetLevel(HAL_485_DISPLAY_CONTRL_PIN, HAL_485_CONTRL_ENABLE_LEVEL);
    }
    else
    {
        HalGPIOSetLevel(HAL_485_DISPLAY_CONTRL_PIN, HAL_485_CONTRL_DISABLE_LEVEL);
    }
}

void hal_ledpanel_pin_config(void)
{
#if 0
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOE, ENABLE );	  //PE15485¿ØÖÆ
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //ÍÆÍìÊä³ö
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE,GPIO_Pin_13);
#endif
    HalGPIOConfig(HAL_485_DISPLAY_CONTRL_PIN, HAL_IO_OUTPUT);
}

void uart3_recv_cb(u8 *data,u16 len)
{
	VTSQueuePush(g_ledpanelRecvQueue,*data);
}

void hal_ledpanel_com_recv_poll(void)
{
	if(VTSQueueCount(g_ledpanelRecvQueue))
	{
		while(1)
		{
			if(VTSQueueIsEmpty(g_ledpanelRecvQueue))
			{
				break;
			}
			unsigned char data = VTSQueueFront(g_ledpanelRecvQueue);
			__disable_irq();
			VTSQueuePop(g_ledpanelRecvQueue);
			__enable_irq();
            if(g_byte_recv_handler)
			    g_byte_recv_handler(data);
		}
	}
}

uint16_t hal_ledpanel_com_send(const void *data, uint16_t len)
{
	//uint16_t i;
	//uint8_t *dataByte = (uint8_t *)data;
	
	hal_ledpanel_send_enable(1);

	HalUartWrite(HAL_UART_DISPLAY_PORT, data, len);

	hal_ledpanel_send_enable(0);
	return len;
}

void set_ledpanel_byte_recv_handler(byte_recv_handler f)
{
	g_byte_recv_handler = f;
}





















