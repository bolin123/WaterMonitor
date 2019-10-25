#include "SensorCom.h"

#define SCOM_DEVICE_MAX_NUM 10

static SComDevice_t g_devices[SCOM_DEVICE_MAX_NUM];
static SComDevice_t *g_currentDevice = NULL;
static uint8_t g_buff[128];

static void lowSend(uint8_t *data, uint16_t len)
{
#if 0
    uint8_t i;
    //SysLog("");
    for(i = 0; i < len; i++)
    {
        SysPrintf("%02x ", data[i]);
    }
    SysPrintf("\n");
#endif    
    if(data)
    {
        HalGPIOSetLevel(HAL_485_SENSOR_CONTRL_PIN, HAL_485_CONTRL_ENABLE_LEVEL);
        HalUartWrite(HAL_UART_SENSOR_PORT, data, len);
        HalGPIOSetLevel(HAL_485_SENSOR_CONTRL_PIN, HAL_485_CONTRL_DISABLE_LEVEL);
    }
}

static unsigned int crc16(uint8_t *buf, uint16_t length)//CRC16计算  
{  
    uint16_t i;  
    uint32_t crc = 0xFFFF;

    while(length--)  
    {
        for(crc ^= *(buf++), i = 0; i < 8; i++)  
        {
            crc = (crc & 0x0001) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);  
        }
    }
    
    return crc;  
}

void SComRawSend(SComDevice_t *device, uint8_t *data, uint16_t len)
{
    if(device)
    {
        lowSend(data, len);
        g_currentDevice = device;
    }
}

void SComModbusSet(SComDevice_t *device, 
    uint8_t cmd, 
    uint16_t startReg, 
    uint16_t regNum, 
    uint8_t *data, 
    uint8_t dlen)
{
    uint8_t buff[128];
    uint8_t i = 0;
    uint16_t crc;

    if(device)
    {
        buff[i++] = device->addr;
        buff[i++] = cmd;
        buff[i++] = (uint8_t)(startReg >> 8);//寄存器起始地址高位
        buff[i++] = (uint8_t)(startReg & 0x00ff);//寄存器起始地址低位
        buff[i++] = (uint8_t)(regNum >> 8);//寄存器数量高位
        buff[i++] = (uint8_t)(regNum & 0x00ff);//寄存器数量低位
        buff[i++] = dlen;//字节数
        memcpy(&buff[i], data, dlen);
        i += dlen;
        
        crc = crc16(buff, i);
        buff[i++] = (uint8_t)(crc & 0x00ff);
        buff[i++] = (uint8_t)(crc >> 8);
        
        lowSend(buff, i);
        g_currentDevice = device;
    }
}

void SComModbusRead(SComDevice_t *device, uint8_t cmd, uint16_t startReg, uint16_t len)
{
    uint8_t buff[64];
    uint8_t i = 0;
    uint16_t crc;

    if(device)
    {
        buff[i++] = device->addr;
        buff[i++] = cmd;
        buff[i++] = (uint8_t)(startReg >> 8);
        buff[i++] = (uint8_t)(startReg & 0x00ff);
        buff[i++] = (uint8_t)(len >> 8);
        buff[i++] = (uint8_t)(len & 0x00ff);
        crc = crc16(buff, i);
        buff[i++] = (uint8_t)(crc & 0x00ff);
        buff[i++] = (uint8_t)(crc >> 8);
        
        lowSend(buff, i);
        g_currentDevice = device;
    }
}

SComDevice_t *SComDeviceRegist(uint8_t addr, CrcEndian_t endian, SComRecv_cb recvCb)
{
    SComDevice_t *device;
    static uint8_t devCount = 0;
    
    if(devCount < SCOM_DEVICE_MAX_NUM)
    {
        device = &g_devices[devCount];
        device->addr = addr;
        device->endian = endian;
        device->recvCb = recvCb;
        devCount++;
        return device;
    }
    return NULL;
}

static void modbusRecv(uint8_t *data, uint16_t len)
{
    uint16_t i;
    uint16_t crc;
    static uint8_t buffCount = 0;
    static uint8_t totalLen = 0;
    static bool readAck = false;

    for(i = 0; i < len; i++)
    {
        g_buff[buffCount++] = data[i];

        if(buffCount >= sizeof(g_buff))
        {
            buffCount = 0;
        }

        if(buffCount == 1)
        {
            if(data[i] != g_currentDevice->addr)
            {
                buffCount = 0;
            }
        }
        else if(buffCount == 2)
        {
            if(data[i] == SCOM_MODBUS_CMD_READ1_REG || data[i] == SCOM_MODBUS_CMD_READ2_REG)
            {
                readAck = true;
            }
            else if(data[i] == SCOM_MODBUS_CMD_SET_REG)
            {
                readAck = false;
                totalLen = 8;
            }
        }
        else if(buffCount == 3)
        {
            if(readAck)
            {
                totalLen = data[i] + 5;//head(2) + len(1) + crc(2)
            }
        }
        else if(buffCount == totalLen) 
        {
            crc = data[i];
            if(g_currentDevice->endian == LBHA)
                crc = (crc << 8) + g_buff[buffCount - 2];
            else
                crc = (g_buff[buffCount - 2] << 8) + crc;
            //SysPrintf("crc:%x\n", crc);
            //SysPrintf("crc16:%x\n", crc16(g_buff, buffCount - 2));
            if(crc == crc16(g_buff, buffCount - 2))
            {
                if(g_currentDevice->recvCb)
                {
                    g_currentDevice->recvCb(&g_buff[1], buffCount - 3);
                }
            }
            totalLen = 0;
            buffCount = 0;
        }
    }
}

static void uartRecv(uint8_t *data, uint16_t len)
{
    if(g_currentDevice)
    {
        if(g_currentDevice->addr != SCOM_MODBUS_INVALED_ADDR)
        {
            modbusRecv(data, len);
        }
        else
        {
            g_currentDevice->recvCb(data, len);
        }
    }
}

static void comConfig(void)
{
    HalUartConfig_t config;

    config.baudrate = 9600;
    config.flowControl = 0;
    config.parity = 0;
    config.wordLength = USART_WordLength_8b;
    config.recvCb = uartRecv;
    HalUartConfig(HAL_UART_SENSOR_PORT, &config);
}

static void ctrlIOConfig(void)
{
#if 0
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOE,GPIO_Pin_15);
#endif
    HalGPIOConfig(HAL_485_SENSOR_CONTRL_PIN, HAL_IO_OUTPUT);
}

void SComInitialize(void)
{
    ctrlIOConfig();
    comConfig();
}

void SComPoll(void)
{
}

