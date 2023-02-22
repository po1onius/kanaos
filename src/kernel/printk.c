#include <stdarg.h>
#include <console.h>
#include <stdio.h>
#include <lock.h>

static int8 buf[1024];
static struct lock print_lock;

void print_lock_init()
{
    extern void printf_lock_init();
    lock_init(&print_lock);
    printf_lock_init();
}


int32 printk(int8* fmt, ...)
{
    va_list args;
    int32 i;

    lock_acquire(&print_lock);
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    console_write(NULL, buf, i);

    lock_release(&print_lock);
    return i;
}