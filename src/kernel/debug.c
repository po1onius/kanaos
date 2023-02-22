#include <debug.h>
#include <stdarg.h>
#include <stdio.h>
#include <printk.h>

static int8 buf[1024];

void debugk(int8 *file, int32 line, const int8 *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    //printk("[%s] [%d] %s", file, line, buf);
}