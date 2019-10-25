#ifndef SYS_COMMAND_H
#define SYS_COMMAND_H

#include "Sys.h"

void SysCmdByteRecv(uint8_t *data, uint16_t len);
void SysCmdInit(void);
void SysCmdPoll(void);

#endif

