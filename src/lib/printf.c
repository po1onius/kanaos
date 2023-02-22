#include <types.h>
#include <stdarg.h>
#include <stdio.h>
#include <syscall.h>
#include <lock.h>

#define BUFSZ 1024

static int8 buf[BUFSZ];
static struct lock lock;

void printf_lock_init()
{
    lock_init(&lock);
}

int32 printf(const int8 *fmt, ...)
{
    va_list args;
    int32 i;
    //lock_acquire(&lock);
    va_start(args, fmt);

    i = vsprintf(buf, fmt, args);

    va_end(args);

    write(stdout, buf, i);
    //lock_release(&lock);
    return i;
}