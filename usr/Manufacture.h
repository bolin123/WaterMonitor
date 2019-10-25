/*******************************************************************************
 * ������أ�
 * �����ӿڣ����ڣ�д���豸ID��Pin�룬��ȡ�豸ID������ģ�飬����ģ��Ap�ȡ�
 ******************************************************************************/

#ifndef MAF_MODE_H
#define MAF_MODE_H
#include "sys.h"


void MAFDataPreprocess(const uint8_t *data, uint16_t len);
void MAFInit(void);


typedef enum
{
    TEST_TYPE_FULL=0,
    TEST_TYPE_RTC,
    TEST_TYPE_I2C,
    TEST_TYPE_ADC,
    TEST_TYPE_WP,
    TEST_TYPE_TH,
    TEST_TYPE_GPRS,
    TEST_TYPE_SD,
    TEST_TYPE_ID,
    TEST_TYPE_NULL
}TestType_t;



#endif // MAF_MODE_H
