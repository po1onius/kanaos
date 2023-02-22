#ifndef __TIME_H__
#define __TIME_H__

#include <types.h>

struct tm
{
    uint32 tm_sec;   // 秒数 [0，59]
    uint32 tm_min;   // 分钟数 [0，59]
    uint32 tm_hour;  // 小时数 [0，59]
    uint32 tm_mday;  // 1 个月的天数 [0，31]
    uint32 tm_mon;   // 1 年中月份 [0，11]
    uint32 tm_year;  // 从 1900 年开始的年数
    uint32 tm_wday;  // 1 星期中的某天 [0，6] (星期天 =0)
    uint32 tm_yday;  // 1 年中的某天 [0，365]
    uint32 tm_isdst; // 夏令时标志
};

uint8 cmos_read(uint8 addr);
void time_read_bcd(struct tm *time);
void time_read(struct tm *time);
time_t mktime(struct tm *time);
void time_init();

#endif