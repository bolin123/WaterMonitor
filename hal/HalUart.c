#include "HalUart.h"
#include "stm32f10x_dma.h"
#include "usart_io.h"

static HalUartConfig_t g_uartConfig[4];
static uint8_t *g_dmaBuff = NULL;
static HalUartDmaRecv_cb g_dmaRecvCb = NULL;
static uint16_t g_dmaLastNum = HAL_GPRS_RECV_QUEUE_LEN;

static void dmaInit(void)
{
    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA1_Channel5);
    USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
    DMA_Cmd(DMA1_Channel5, DISABLE);

    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)g_dmaBuff;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = HAL_GPRS_RECV_QUEUE_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);
    
    DMA_ClearFlag(DMA1_FLAG_TC5);
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE); 
    
    DMA_Cmd(DMA1_Channel5, ENABLE); 
}

void HalUartDmaInit(uint8_t uart, HalUartConfig_t *config, uint8_t *buff, HalUartDmaRecv_cb handle)
{
    USART_InitTypeDef uartcfg;

    if(HAL_UART_GPRS_PORT == uart)
    {
        g_dmaBuff = buff;
        g_dmaRecvCb = handle;
        memset(buff, 0, HAL_GPRS_RECV_QUEUE_LEN);

        USART_Cmd(USART1, DISABLE);
        USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
        
        uartcfg.USART_BaudRate = config->baudrate;
        uartcfg.USART_WordLength = config->wordLength;
        uartcfg.USART_Parity = config->parity;
        uartcfg.USART_StopBits = USART_StopBits_1;
        uartcfg.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        uartcfg.USART_HardwareFlowControl = config->flowControl;

        USART_Init(USART1, &uartcfg);
        
        dmaInit();
        g_dmaLastNum = HAL_GPRS_RECV_QUEUE_LEN;
        USART_Cmd(USART1, ENABLE);
    }
}

void HalUartConfig(uint8_t uart, HalUartConfig_t *config)
{   
    USART_InitTypeDef uartcfg;
    USART_TypeDef *uartNo;
    
    USART_StructInit(&uartcfg);
    if(0 == uart)
    {
        uartNo = USART1;
    }
    else if(1 == uart)
    {
        uartNo = USART2;
    }
    else if(2 == uart)
    {
        uartNo = USART3;
    }
    else if(3 == uart)
    {
        SimulateUartConfig(uart, config);
        return;
    }
    else
    {
        return;
    }
    g_uartConfig[uart] = *config;

    USART_Cmd(uartNo, DISABLE);
    USART_ITConfig(uartNo, USART_IT_RXNE, DISABLE);
    
    uartcfg.USART_BaudRate = config->baudrate;
    uartcfg.USART_WordLength = config->wordLength;
    uartcfg.USART_Parity = config->parity;
    uartcfg.USART_StopBits = USART_StopBits_1;
    uartcfg.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    uartcfg.USART_HardwareFlowControl = config->flowControl;

    USART_Init(uartNo, &uartcfg);

    USART_ClearITPendingBit(uartNo, USART_IT_RXNE);
    USART_ITConfig(uartNo, USART_IT_RXNE, ENABLE);
    
    USART_Cmd(uartNo, ENABLE);
}

void HalUartInitialize(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
#if 1
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    GPIO_InitTypeDef GPIO_InitStructure;

    //uart1 io
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;  //TX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_10;  //RX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //uart2 io
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;  //TX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;  //RX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //uart3 io
    GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;  //TX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
     
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;  //RX
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void HalUartPoll(void)
{
    uint16_t tmp;
    
    if(DMA_GetCurrDataCounter(DMA1_Channel5) != g_dmaLastNum)//ʣ�����������
    {
        tmp = DMA_GetCurrDataCounter(DMA1_Channel5);

        if(g_dmaRecvCb != NULL)
        {
            g_dmaRecvCb(g_dmaLastNum - tmp);
        }
        g_dmaLastNum = tmp;
    }
    if(DMA_GetFlagStatus(DMA1_FLAG_TC5) == SET)
    {
        DMA_ClearFlag(DMA1_FLAG_TC5);

        if(g_dmaRecvCb != NULL)
        {
            g_dmaRecvCb(g_dmaLastNum);
        }
        g_dmaLastNum = HAL_GPRS_RECV_QUEUE_LEN;
    }
}

void HalUartWrite(uint8_t uart, const uint8_t *data, uint16_t len)
{
    uint16_t i;
    USART_TypeDef *uartNo;
    if(3 == uart)
    {
        SimUart_send_buf((uint8_t *)data, len);
        return;
    }    
    else if(0 == uart)
    {
        uartNo = USART1;
    }
    else if(1 == uart)
    {
        uartNo = USART2;
    }
    else
    {
        uartNo = USART3;
    }

    for(i = 0; i < len; i++)
    {
        USART_SendData(uartNo, (uint16_t)data[i]);
        while(USART_GetFlagStatus(uartNo, USART_FLAG_TC) == RESET);
    }
    
}

#if 1
void USART1_IRQHandler(void)
{
    uint8_t data;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
        data = USART_ReceiveData(USART1);
        if(g_uartConfig[0].recvCb != NULL)
        {
            g_uartConfig[0].recvCb(&data, 1);
        }
    }
}
#endif

void USART2_IRQHandler(void)
{
    uint8_t data;
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        data = USART_ReceiveData(USART2);
        if(g_uartConfig[1].recvCb != NULL)
        {
            g_uartConfig[1].recvCb(&data, 1);
        }
    }
}

void USART3_IRQHandler(void)
{
    uint8_t data;
    if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        data = USART_ReceiveData(USART3);
        if(g_uartConfig[2].recvCb != NULL)
        {
            g_uartConfig[2].recvCb(&data, 1);
        }
    }
}


