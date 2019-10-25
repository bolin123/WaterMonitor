#ifndef YM_PROPERTY_MANAGER_H
#define YM_PROPERTY_MANAGER_H

const char *YMPMProperties2Text(void);
int YMPMRegister(const char *name, const char *flagName);
int YMPMSetValue(const char * name, float value, unsigned char flag);

void YMPMInitialize(void);
void YMPMPoll(void);

#endif

