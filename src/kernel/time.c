#include <time.h>
#include <debug.h>
#include <stdlib.h>
#include <io.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define CMOS_ADDR 0x70 // CMOS 地址寄存器
#define CMOS_DATA 0x71 // CMOS 数据寄存器

// 下面是 CMOS 信息的寄存器索引
#define CMOS_SECOND 0x00  // (0 ~ 59)
#define CMOS_MINUTE 0x02  // (0 ~ 59)
#define CMOS_HOUR 0x04    // (0 ~ 23)
#define CMOS_WEEKDAY 0x06 // (1 ~ 7) 星期天 = 1，星期六 = 7
#define CMOS_DAY 0x07     // (1 ~ 31)
#define CMOS_MONTH 0x08   // (1 ~ 12)
#define CMOS_YEAR 0x09    // (0 ~ 99)
#define CMOS_CENTURY 0x32 // 可能不存在
#define CMOS_NMI 0x80

#define MINUTE 60          // 每分钟的秒数
#define HOUR (60 * MINUTE) // 每小时的秒数
#define DAY (24 * HOUR)    // 每天的秒数
#define YEAR (365 * DAY)   // 每年的秒数，以 365 天算

// 每个月开始时的已经过去天数
static uint32 month[13] = {
    0, // 这里占位，没有 0 月，从 1 月开始
    0,
    (31),
    (31 + 29),
    (31 + 29 + 31),
    (31 + 29 + 31 + 30),
    (31 + 29 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
    (31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)};


time_t startup_time;

// 这里生成的时间可能和 UTC 时间有出入
// 与系统具体时区相关，不过也不要紧，顶多差几个小时
time_t mktime(struct tm *time)
{
    time_t res;
    uint32 year; // 1970 年开始的年数
    // 下面从 1900 年开始的年数计算
    if (time->tm_year >= 70)
    {
        year = time->tm_year - 70;
    }
    else
    {
        year = time->tm_year - 70 + 100;
    }
        

    // 这些年经过的秒数时间
    res = YEAR * year;

    // 已经过去的闰年，每个加 1 天
    res += DAY * ((year + 1) / 4);

    // 已经过完的月份的时间
    res += month[time->tm_mon] * DAY;

    // 如果 2 月已经过了，并且当前不是闰年，那么减去一天
    if (time->tm_mon > 2 && ((year + 2) % 4))
    {
        res -= DAY;
    }
        
    // 这个月已经过去的天
    res += DAY * (time->tm_mday - 1);

    // 今天过去的小时
    res += HOUR * time->tm_hour;

    // 这个小时过去的分钟
    res += MINUTE * time->tm_min;

    // 这个分钟过去的秒
    res += time->tm_sec;

    return res;
}

uint32 get_yday(struct tm *time)
{
    uint32 res = month[time->tm_mon]; // 已经过去的月的天数
    res += time->tm_mday;          // 这个月过去的天数

    uint32 year;
    if (time->tm_year >= 70)
    {
        year = time->tm_year - 70;
    }
    else
    {
        year = time->tm_year - 70 + 100;
    }
        

    // 如果不是闰年，并且 2 月已经过去了，则减去一天
    // 注：1972 年是闰年，这样算不太精确，忽略了 100 年的平年
    if ((year + 2) % 4 && time->tm_mon > 2)
    {
        res -= 1;
    }

    return res;
}

uint8 cmos_read(uint8 addr)
{
    outb(CMOS_ADDR, CMOS_NMI | addr);
    return inb(CMOS_DATA);
};

void time_read_bcd(struct tm *time)
{
    // CMOS 的访问速度很慢。为了减小时间误差，在读取了下面循环中所有数值后，
    // 若此时 CMOS 中秒值发生了变化，那么就重新读取所有值。
    // 这样内核就能把与 CMOS 的时间误差控制在 1 秒之内。
    do
    {
        time->tm_sec = cmos_read(CMOS_SECOND);
        time->tm_min = cmos_read(CMOS_MINUTE);
        time->tm_hour = cmos_read(CMOS_HOUR);
        time->tm_wday = cmos_read(CMOS_WEEKDAY);
        time->tm_mday = cmos_read(CMOS_DAY);
        time->tm_mon = cmos_read(CMOS_MONTH);
        time->tm_year = cmos_read(CMOS_YEAR);
    } while (time->tm_sec != cmos_read(CMOS_SECOND));
}

void time_read(struct tm *time)
{
    time_read_bcd(time);
    time->tm_sec = bcd_to_bin(time->tm_sec);
    time->tm_min = bcd_to_bin(time->tm_min);
    time->tm_hour = bcd_to_bin(time->tm_hour);
    time->tm_wday = bcd_to_bin(time->tm_wday);
    time->tm_mday = bcd_to_bin(time->tm_mday);
    time->tm_mon = bcd_to_bin(time->tm_mon);
    time->tm_year = bcd_to_bin(time->tm_year);
    time->tm_yday = get_yday(time);
    time->tm_isdst = -1;
}

void time_init()
{
    struct tm time;
    time_read(&time);
    startup_time = mktime(&time);
    LOGK("startup time: %d-%02d-%02d %02d:%02d:%02d\n",
         time.tm_year,
         time.tm_mon,
         time.tm_mday,
         time.tm_hour,
         time.tm_min,
         time.tm_sec);
}