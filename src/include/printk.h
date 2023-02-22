#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <types.h>

void print_lock_init();
int32 printk(int8* fmt, ...);

#endif