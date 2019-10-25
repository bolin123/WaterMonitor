#ifndef YM_CTYPE_H
#define YM_CTYPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "YMOption.h"

#undef YmBool
#define YmBool unsigned char
#undef YmUint8_t 
#define YmUint8_t unsigned char
#undef YmUint16_t 
#define YmUint16_t unsigned short
#undef YmUint32_t
#define YmUint32_t unsigned int
#undef YmInt8_t 
#define YmInt8_t signed char
#undef YmInt16_t 
#define YmInt16_t signed short
#undef YmInt32_t
#define YmInt32_t signed int
#undef YmUint64_t
#define YmUint64_t unsigned long long int
#undef YmInt64_t
#define YmInt64_t long long int

#undef YmTrue
#define YmTrue (1)
#undef YmFalse
#define YmFalse (0)

#define YmNULL ((void *)0)

void *SysMalloc(YmUint16_t size);
#define YmMalloc(n) SysMalloc(n)

#include "YMUserOption.h"

#endif // !YM_CTYPE_H
