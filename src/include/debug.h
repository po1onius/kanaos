
#ifndef __DEBUG_H__
#define __DEBUG_H__
#include <types.h>

void debugk(int8 *file, int32 line, const int8 *fmt, ...);

#define BMB asm volatile("xchgw %bx, %bx") // bochs magic breakpoint
#define DEBUGK(fmt, args...) debugk(__BASE_FILE__, __LINE__, fmt, ##args)

#endif
