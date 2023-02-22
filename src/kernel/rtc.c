#include <rtc.h>
#include <types.h>
#include <debug.h>
#include <io.h>
#include <time.h>
#include <stdlib.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

#define CMOS_SECOND 0x01
#define CMOS_MINUTE 0x03
#define CMOS_HOUR 0x05

#define CMOS_A 0x0a
#define CMOS_B 0x0b
#define CMOS_C 0x0c
#define CMOS_D 0x0d
#define CMOS_NMI 0x80

// 写 cmos 寄存器的值
void cmos_write(uint8 addr, uint8 value)
{
    outb(CMOS_ADDR, CMOS_NMI | addr);
    outb(CMOS_DATA, value);
}

// 实时时钟中断处理函数
void rtc_intr_handler()
{
    // 读 CMOS 寄存器 C，允许 CMOS 继续产生中断
    cmos_read(CMOS_C);

    //set_alarm(1);

    LOGK("rtc handler\n");
}

// 设置 secs 秒后发生实时时钟中断
void set_alarm(uint32 secs)
{
    struct tm time;
    time_read(&time);

    uint8 sec = secs % 60;
    secs /= 60;
    uint8 min = secs % 60;
    secs /= 60;
    uint32 hour = secs;

    time.tm_sec += sec;
    if (time.tm_sec >= 60)
    {
        time.tm_sec %= 60;
        time.tm_min += 1;
    }

    time.tm_min += min;
    if (time.tm_min >= 60)
    {
        time.tm_min %= 60;
        time.tm_hour += 1;
    }

    time.tm_hour += hour;
    if (time.tm_hour >= 24)
    {
        time.tm_hour %= 24;
    }

    cmos_write(CMOS_HOUR, bin_to_bcd(time.tm_hour));
    cmos_write(CMOS_MINUTE, bin_to_bcd(time.tm_min));
    cmos_write(CMOS_SECOND, bin_to_bcd(time.tm_sec));
}

void rtc_init()
{
    // cmos_write(CMOS_B, 0b01000010); // 打开周期中断
    cmos_write(CMOS_B, 0b00100010); // 打开闹钟中断
    cmos_read(CMOS_C); // 读 C 寄存器，以允许 CMOS 中断

    // 设置中断频率
    outb(CMOS_A, (inb(CMOS_A) & 0xf) | 0b1110);
    //set_alarm(2);
}
