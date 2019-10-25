#ifndef SENSOR_COM_H
#define SENSOR_COM_H

#include "Sys.h"

#define SCOM_MODBUS_INVALED_ADDR 0x00

#define SCOM_MODBUS_CMD_READ1_REG 0x03
#define SCOM_MODBUS_CMD_READ2_REG 0x04
#define SCOM_MODBUS_CMD_SET_REG   0x10

typedef enum
{
    LBHA=0,
    HBLA
}CrcEndian_t;

typedef void (*SComRecv_cb)(uint8_t *data, uint16_t len);
typedef struct
{
    uint8_t addr;
    CrcEndian_t endian;
    SComRecv_cb recvCb;
}SComDevice_t;

void SComRawSend(SComDevice_t *device, uint8_t *data, uint16_t len);
void SComModbusRead(SComDevice_t *device, uint8_t cmd, uint16_t startReg, uint16_t len);
void SComModbusSet(SComDevice_t *device, 
    uint8_t cmd, 
    uint16_t startReg, 
    uint16_t regNum, 
    uint8_t *data, 
    uint8_t dlen);

SComDevice_t *SComDeviceRegist(uint8_t addr, CrcEndian_t endian, SComRecv_cb recvCb);
void SComInitialize(void);
void SComPoll(void);

#endif

