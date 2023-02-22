#include <clock.h>
#include <types.h>
#include <speaker.h>
#include <thread.h>
#include <assert.h>
#include <io.h>
#include <printk.h>

static uint32 volatile jiffies = 0;

void pit_init()
{
    outb(PIT_CTRL_REG, 0b00110100);
    outb(PIT_CHAN0_REG, CLOCK_COUNTER & 0xff);
    outb(PIT_CHAN0_REG, (CLOCK_COUNTER >> 8) & 0xff);
}

uint32 get_jiffies()
{
    return jiffies;
}

extern time_t startup_time;
time_t sys_time()
{
    return startup_time + (jiffies * JIFFY) / 1000;
}

void clock_intr_handler()
{
    jiffies++;
    beeping_check();
    //if(jiffies % 200 == 0)
    //{
    //    start_beep();
    //}
    //printk("clock:%d\n", jiffies);
    //schedule();
    struct task_struct* cur = running_thread();
    assert(cur->stack_magic == STACK_MAGIC);
    cur->abs_ticks++;
    if(cur->ticks == 0)
    {
        schedule();
    }
    else
    {
        cur->ticks--;
    }
}
