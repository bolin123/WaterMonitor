#include "delay.h"

void delay_init(void)
{

}
void delay_ms(u16 nms)
{
	u16 i=0;
	while(nms--)
	{
		i = 8000; //延时100ms效果精确
		while(i--)
		{
			//__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;
			//__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;__NOP;
		}
	}
}
void delay_us(uint32_t us)
	{
	uint32_t temp;
	uint8_t radix = 72-1;
	
	SysTick->LOAD = us*radix;
	SysTick->VAL=0x00;
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; 
	do  
    {  
        temp = SysTick->CTRL;  
	}
    while(temp&0x01&&!(temp&(1<<16)));
	SysTick->CTRL &= 0xfffffff8;  
    SysTick->VAL = 0x00;  
}
 
