#ifndef _LEDPANEL_HARDWARE_H__
#define _LEPANEL_HARDWARE_H__

#include "stm32f10x.h"

typedef void (*byte_recv_handler)(unsigned char);

void hal_ledpanel_pin_config(void);
void hal_ledpanel_com_config(uint16_t baud);

void hal_ledpanel_com_recv_poll(void);
uint16_t hal_ledpanel_com_send(const void *data, uint16_t len);
void set_ledpanel_byte_recv_handler(byte_recv_handler f);

//void hal_modbus_power_switch(uint8_t on);
void hal_ledpanel_send_enable(uint8_t enable);


#endif
