#ifndef _SD_API_H__
#define _SD_API_H__


#include "sdio_sdcard.h"
#include "ff.h"
#include "stdio.h"
#include "string.h"

void sd_detectPin_init(void);

void sd_record(char *path,float array[],uint8_t num);

#define AVERAGE_DATA_FILE_PATH					"1:/AD.TXT"
#define REALTIME_DATA_FILE_PATH					"1:/RD.TXT"

/*
#define CONTINUOUS_1MIN_FILE_PATH					"1:/2m.TXT"
#define CONTINUOUS_1MIN_SECDATA_FILE_PATH "1:/1s.TXT" //文件名最长8字符
#define PERIODIC_5MIN_FILE_PATH						"1:/5m.TXT"
#define PERIODIC_5MIN_SECDATA_FILE_PATH 	"1:/5s.TXT"
#define PERIODIC_10MIN_FILE_PATH					"1:/10.TXT"
#define PERIODIC_10MIN_SECDATA_FILE_PATH  "1:/11.TXT"
#define PERIODIC_20MIN_FILE_PATH					"1:/20.TXT"
#define PERIODIC_20MIN_SECDATA_FILE_PATH  "1:/21.TXT"
#define PERIODIC_30MIN_FILE_PATH					"1:/30.TXT" 
#define PERIODIC_30MIN_SECDATA_FILE_PATH  "1:/31.TXT"
#define TRACE_FILE_PATH										"1:/tc.TXT"
#define GPRS_FILE_PATH										"1:/gs.TXT"
#define SECOND_DATA_FILE_PATH							"1:/secdata.TXT"
*/

int8_t IsSDHalError(void);

int HalFileGetNextMessage(uint8_t id, char *data, uint16_t dlen);
int HalFileDelLastMessage(uint8_t id);
int HalFileInsertMessage(uint8_t id, const char* msg);
void HalFileUpdateInfo(void);
int HalFileRecord(char *data);



#endif
