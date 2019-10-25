/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: Header file of existing USB MSD control module */
//#include "atadrive.h"	/* Example: Header file of existing ATA harddisk control module */
#include "sdio_sdcard.h"		/* Example: Header file of existing MMC/SDC contorl module */
#include "stdio.h"

#define BLOCK_SIZE            512 /* Block Size in Bytes */

/* Definitions of physical drive number for each drive */
#define ATA		0	/* Example: Map ATA harddisk to physical drive 0 */
#define MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case ATA :

		// translate the reslut code here

		return stat;

	case MMC :

		// translate the reslut code here

		return RES_OK;

	case USB :

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	SD_Error result;

	switch (pdrv) {
	case ATA :

		// translate the reslut code here

		return stat;

	case MMC :
		result = SD_Init();
		if (result!=SD_OK )
		{
			return STA_NOINIT;
		}
		else
		{
			return RES_OK;
		}

	case USB :

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_OK;

	switch (pdrv) {
	case ATA :
		// translate the arguments here

		// translate the reslut code here

		return res;

	case MMC :
		if (count > 1)
		{
			SD_ReadMultiBlocks(buff, sector*BLOCK_SIZE, BLOCK_SIZE, count);
		
				  /* Check if the Transfer is finished */
			 SD_WaitReadOperation();  //循环查询dma传输是否结束
		
			/* Wait until end of DMA transfer */
			while(SD_GetStatus() != SD_TRANSFER_OK);
		
		}
		else
		{
			
			SD_ReadBlock(buff, sector*BLOCK_SIZE, BLOCK_SIZE);
		
				  /* Check if the Transfer is finished */
			 SD_WaitReadOperation();  //循环查询dma传输是否结束
		
			/* Wait until end of DMA transfer */
			while(SD_GetStatus() != SD_TRANSFER_OK);
		
		}
		return RES_OK;


	case USB :
		// translate the arguments here

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_OK;

	switch (pdrv) {
	case ATA :
		// translate the arguments here

		// translate the reslut code here

		return res;

	case MMC :
		if (count > 1)
		{
            SD_Error errorstatus = SD_OK;
            errorstatus = SD_WriteMultiBlocks((uint8_t *)buff, sector*BLOCK_SIZE, BLOCK_SIZE, count);
            if(errorstatus != SD_OK)
                return RES_ERROR;
			  /* Check if the Transfer is finished */
			errorstatus = SD_WaitWriteOperation();	   //等待dma传输结束
            if(errorstatus != SD_OK)
                return RES_ERROR;
            uint32_t timeout=0;
			while((timeout<0x2fff)&&(SD_GetStatus() != SD_TRANSFER_OK)) //等待sdio到sd卡传输结束
            {
                timeout++;
            }
            if(timeout >= 0x2fff)
            {
                printf("M_timeout=%d",timeout);
                return RES_ERROR;
            }
		}
		else
		{
            SD_Error errorstatus = SD_OK;
            errorstatus = SD_WriteBlock((uint8_t *)buff,sector*BLOCK_SIZE, BLOCK_SIZE);
			if(errorstatus != SD_OK)
                return RES_ERROR;
			  /* Check if the Transfer is finished */
			errorstatus = SD_WaitWriteOperation();	   //等待dma传输结束
			if(errorstatus != SD_OK)
                return RES_ERROR;
            uint32_t timeout=0;
			while((timeout<0x2fff)&&(SD_GetStatus() != SD_TRANSFER_OK)) //等待sdio到sd卡传输结束
            {
                timeout++;
            }
            if(timeout >= 0x2fff)
            {
                printf("S_timeout=%d",timeout);
                return RES_ERROR;
            }

		}
		return RES_OK;


	case USB :
		// translate the arguments here

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_OK;

	switch (pdrv) {
	case ATA :

		// Process of the command for the ATA drive

		return res;

	case MMC :

		// Process of the command for the MMC/SD card

		return res;

	case USB :

		// Process of the command the USB drive

		return res;
	}

	return RES_PARERR;
}
#endif

/*-----------------------------------------------------------------------*/
/* Get current time                                                      */
/*-----------------------------------------------------------------------*/ 
DWORD get_fattime(void)
{

 	return 0;

} 

