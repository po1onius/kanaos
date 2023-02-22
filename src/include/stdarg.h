#ifndef __STDARG_H__
#define __STDARG_H__

#include <types.h>

typedef int8* va_list;

#define va_start(ap, v) (ap = (va_list)&v + sizeof(int8*))
#define va_arg(ap, t) (*(t*)((ap += sizeof(int8*)) - sizeof(int8*)))
#define va_end(ap) (ap = (va_list)0)

#endif