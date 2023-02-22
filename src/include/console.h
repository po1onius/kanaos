#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include <types.h>

void console_init();
void console_clear();
void console_write(void* dev, int8* str, size_t size);

#endif