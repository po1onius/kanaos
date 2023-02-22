#include <kanaos.h>
#include <io.h>
#include <string.h>
#include <console.h>
#include <stdio.h>
#include <debug.h>
#include <assert.h>
#include <gdt.h>
#include <intr.h>
#include <stdlib.h>
#include <time.h>
#include <rtc.h>
#include <clock.h>
#include <speaker.h>
#include <thread.h>
#include <printk.h>
#include <syscall.h>
#include <lock.h>
#include <keyboard.h>
#include <tss.h>
#include <process.h>
#include <ide.h>
#include <device.h>
#include <buffer.h>
#include <fs.h>


void sh(void)
{
    extern void osh_main();
    uint32 ret_pid = fork();
    while(1)
    {
        if(ret_pid)
        {
            printf("i am father, my pid is %d, child pid is %d\n", getpid(), ret_pid);
            while(1);
        }
        else
        {
            printf("i am child, my pid is %d, ret pid is %d\n", getpid(), ret_pid);
            osh_main();
            //while(1);
        }
    }
}



void kernel_init()
{
    gdt_init();
    tss_init();
    syscall_init();
    print_lock_init();
    intr_init();

    //
    pit_init();//时钟中断
    speaker_init();//蜂鸣器


    time_init();
    rtc_init();


    keyboard_init();
    thread_init();
    ide_init();

    buffer_init();
    file_init();
    inode_init();
    super_init();
    process_execute(sh, "sh");
    
    intr_enable();
    while(1);
}