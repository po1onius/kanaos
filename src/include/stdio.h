#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <types.h>

int32 vsprintf(int8 *buf, const int8 *fmt, va_list args);
int32 sprintf(int8 *buf, const int8 *fmt, ...);
int32 printf(const int8 *fmt, ...);

#endif