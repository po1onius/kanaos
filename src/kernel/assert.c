#include <assert.h>
#include <stdarg.h>
#include <types.h>
#include <stdio.h>
#include <printk.h>

static int8 buf[1024];

// 强制阻塞
static void spin(int8 *name)
{
    printk("spinning in %s ...\n", name);
    while (true);
}

void assertion_failure(int8 *exp, int8 *file, int8 *base, int32 line)
{
    printk(
        "\n--> assert(%s) failed!!!\n"
        "--> file: %s \n"
        "--> base: %s \n"
        "--> line: %d \n",
        exp, file, base, line);

    spin("assertion_failure()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}

void panic(const int8 *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int32 i = vsprintf(buf, fmt, args);
    va_end(args);

    printk("!!! panic !!!\n--> %s \n", buf);
    spin("panic()");

    // 不可能走到这里，否则出错；
    asm volatile("ud2");
}