#ifndef __RTC_H__
#define __RTC_H__

#include <types.h>

void rtc_intr_handler();
void rtc_init();
void set_alarm(uint32 secs);

#endif