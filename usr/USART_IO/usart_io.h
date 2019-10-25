#ifndef _USART_IO_H_
#define _USART_IO_H_

#include "stm32f10x.h"
#include "HalUart.h"


void SimUart_send_buf(uint8_t *_ucaBuf, uint16_t _usLen);
void SimulateUartConfig(uint8_t uart, HalUartConfig_t *config);

void usartIO_status_printf(void);

void IO_UART_TEST(void);




#endif
