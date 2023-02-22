#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <types.h>

#define PIT_CHAN0_REG 0X40
#define PIT_CHAN2_REG 0X42
#define PIT_CTRL_REG 0X43

#define HZ 100
#define OSCILLATOR 1193182
#define CLOCK_COUNTER (OSCILLATOR / HZ)
#define JIFFY (1000 / HZ)


time_t sys_time();
void pit_init();
void clock_intr_handler();
uint32 get_jiffies();

#endif

