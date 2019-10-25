#ifndef _LED_SHOW_H_
#define _LED_SHOW_H_

#include "stm32f10x.h"



//int led_config(LED_config_range_t type);
//void LED_del_file(u16 mainboard_id,u16 controller_id);
//void reset_led(void);
//void LED_write_data(u16 mainboard_id,u16 controller_id);
//void LED_start_write_data(u16 mainboard_id,u16 controller_id);
//void LED_rt_show(u16 mainboard_id,u16 controller_id);
//void LED_ping(u16 mainboard_id,u16 controller_id);
//void restart_led(void);
//u8 is_getLEDAck(u8 tick_100ms);


void led_panel_poll(void);
void led_panel_init(void);








#endif


