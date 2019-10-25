#include "HalFlash.h"
#include "stm32f10x_flash.h"

#define FLASH_CACHE_LEN  512

//#define SECTOR_START_ADDR(addr) ((addr) & (~(HAL_FLASH_BLOCK_SIZE - 1)))
#define SPI_ADDR_TO_SECTOR(addr) ((addr) / HAL_FLASH_BLOCK_SIZE)
//#define ADDR_TO_SECTOR_OFFSET(addr) ((addr) - SECTOR_START_ADDR(addr))


static int flashRead(uint32_t addr, uint8_t *buf, uint16_t len);
static void flashWrite(uint32_t addr, uint8_t *buf, uint16_t len);
static void flashPageErase(uint32_t addr);


void HalFlashInitialize(void)
{

}


void HalFlashPoll(void)
{

}


void HalFlashWrite(uint32_t addr, const void *data, uint32_t len)
{
	unsigned char secBuf[FLASH_CACHE_LEN];
    unsigned char count=0, i=0;
    uint16_t tailLen=0;
    uint32_t startAddr = addr;
	uint8_t * dataByte = (uint8_t *)data;
    
    if((addr%FLASH_CACHE_LEN)&&(len > FLASH_CACHE_LEN))
    {
        uint32_t secStartAddr = addr & (~(FLASH_CACHE_LEN-1));
        uint16_t remainLen = secStartAddr + FLASH_CACHE_LEN - addr;
        uint16_t existLen = FLASH_CACHE_LEN - remainLen;
        memset(secBuf, 0, FLASH_CACHE_LEN);
        flashRead(secStartAddr, secBuf, existLen);        
        memcpy(secBuf + existLen, dataByte, remainLen);
        //flashWrite(secStartAddr, secBuf, FLASH_CACHE_LEN);
        HalFlashWriteInPage(secStartAddr, secBuf, FLASH_CACHE_LEN);
        len -= remainLen;
        dataByte += remainLen;
        startAddr += remainLen;
    }
    
	count = len / FLASH_CACHE_LEN;
    tailLen = len % FLASH_CACHE_LEN; 

    for(i = 0; i < count; i++)
    {
        memcpy(secBuf, dataByte + i * FLASH_CACHE_LEN, FLASH_CACHE_LEN);
        flashWrite(startAddr + i * FLASH_CACHE_LEN, secBuf, FLASH_CACHE_LEN);
    }
    
    //last data
    if(tailLen)
    {
        memset(secBuf, 0xff, FLASH_CACHE_LEN);
        memcpy(secBuf, dataByte + i * FLASH_CACHE_LEN, tailLen);
        flashWrite(startAddr + i * FLASH_CACHE_LEN, secBuf, FLASH_CACHE_LEN);
    }

}

void HalFlashWriteInPage(uint32_t addr, const void *data, uint32_t len)
{
    uint8_t n = 0;
    uint16_t i, j = 0;
    uint16_t offset = ADDR_TO_SECTOR_OFFSET(addr);
    unsigned char secBuf[FLASH_CACHE_LEN];
    
	HalFlashErase(HAL_DEVICE_RESERVE_ADDR);
    for(i = 0; i < HAL_FLASH_BLOCK_SIZE / sizeof(secBuf); i++)
    {
        HalFlashRead(SECTOR_START_ADDR(addr) + i * sizeof(secBuf), (uint8_t *)secBuf, sizeof(secBuf));
        HalFlashWrite(HAL_DEVICE_RESERVE_ADDR + i * sizeof(secBuf),(uint8_t *)secBuf, sizeof(secBuf));
    }

    HalFlashErase(SECTOR_START_ADDR(addr));
    for(i = 0; i < HAL_FLASH_BLOCK_SIZE; i++)
    {
        if(i < offset || i >= (offset + len))
        {
            HalFlashRead(HAL_DEVICE_RESERVE_ADDR + i, &secBuf[j++], 1);
        }
        else
        {
            secBuf[j++] = ((uint8_t *)data)[i - offset];
        }

        if(j == sizeof(secBuf))
        {
            HalFlashWrite(SECTOR_START_ADDR(addr) + n * sizeof(secBuf), \
                (uint8_t *)secBuf, \
                sizeof(secBuf));   
            j = 0;
            n++;
        }
    }
    #if 0
	HalFlashRead(SECTOR_START_ADDR(addr), (uint8_t *)secBuf, HAL_FLASH_BLOCK_SIZE);
	memcpy(secBuf + ADDR_TO_SECTOR_OFFSET(addr), data, len);
	HalFlashErase(SECTOR_START_ADDR(addr));
	HalFlashWrite(SECTOR_START_ADDR(addr),(uint8_t *)secBuf, HAL_FLASH_BLOCK_SIZE);	
    #endif
}

void HalFlashErase(uint32_t addr)
{    
    flashPageErase(addr);
}

void HalFlashRead(uint32_t addr, void *buf, uint32_t bufSize)
{
    flashRead(addr, (uint8_t *)buf, bufSize);
}

static void flashPageErase(uint32_t addr)
{   
    uint8_t i;
    FLASH_Unlock();  
    for(i=0; i<10; i++)
    {
        FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
        if(FLASH_ErasePage(addr) == FLASH_COMPLETE)
        {
            break;
        }
    }
    
    FLASH_Lock();
}


static void flashWrite(uint32_t addr, uint8_t *buf, uint16_t len)
{
	uint32_t g_sun = 0x00000000;
	uint32_t Address = addr;
	uint8_t num = 0;
	uint8_t *str = buf;
	int i,j;
	
	FLASH_Unlock();	
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR); 

	if(buf != 0)
	{
		for(j=len/4; j>0; j--)
		{
			g_sun = 0;
			if(j==0)
			{
				num = len%4;
				switch(num)
				{
					case 1:
					case 2:
					case 3:		
					for(i=num-1 ;i >=0 ;i--)
					{
						g_sun |= str[i];
						if(i > 1)
						{
							g_sun = (g_sun << 8);
						}
						else
						{
							g_sun = (g_sun << 8*i);
						}
					}		
						break;
				}
			}
			else 
			{
					for(i = 3; i >= 1; i--)
					{
						g_sun |= str[i];
						g_sun = (g_sun << 8); 
					}		
					g_sun |= str[0];
			}
			str =str+4;
			if (FLASH_ProgramWord(Address, g_sun) == FLASH_COMPLETE)
			{
				Address = Address + 4;
			}
			else
			{ 
			}
		}
	}
	
	FLASH_Lock();
}

static int flashRead(uint32_t addr, uint8_t *buf, uint16_t len) 
{
	int i = 0;
	memcpy(buf, (const void *)addr, len);
	return i;

}

void FLASH_IRQHandler(void)
{
	printf("%s Entry!!!\n", __func__);
	//while(1);
}


