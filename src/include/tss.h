#ifndef __TSS_H__
#define __TSS_H__

#include <types.h>
#include <gdt.h>
#include <thread.h>

#define TSS_IDX 3
#define SELECTOR_TSS ((TSS_IDX << 3) + (TI_GDT << 2 ) + RPL0)

struct tss
{
    uint32 backlink; // 前一个任务的链接，保存了前一个任状态段的段选择子
    uint32 esp0;     // ring0 的栈顶地址
    uint32 ss0;      // ring0 的栈段选择子
    uint32 esp1;     // ring1 的栈顶地址
    uint32 ss1;      // ring1 的栈段选择子
    uint32 esp2;     // ring2 的栈顶地址
    uint32 ss2;      // ring2 的栈段选择子
    uint32 cr3;
    uint32 eip;
    uint32 flags;
    uint32 eax;
    uint32 ecx;
    uint32 edx;
    uint32 ebx;
    uint32 esp;
    uint32 ebp;
    uint32 esi;
    uint32 edi;
    uint32 es;
    uint32 cs;
    uint32 ss;
    uint32 ds;
    uint32 fs;
    uint32 gs;
    uint32 ldtr;          // 局部描述符选择子
    uint16 trace : 1;     // 如果置位，任务切换时将引发一个调试异常
    uint16 reversed : 15; // 保留不用
    uint16 iobase;        // I/O 位图基地址，16 位从 TSS 到 IO 权限位图的偏移
    uint32 ssp;           // 任务影子栈指针
};

void tss_init();
void update_tss_esp(struct task_struct* pthread);

#endif
