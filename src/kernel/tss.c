#include <tss.h>
#include <gdt.h>
#include <string.h>
#include <memory.h>

static struct tss tss;


void tss_init()
{
    memset(&tss, 0, sizeof(struct tss));
    tss.ss0 = SELECTOR_K_DATA;
    tss.iobase = sizeof(tss);
    struct descriptor* tss_e = gdt + TSS_IDX;
    make_gdt_desc(tss_e, (uint32)&tss, sizeof(tss) - 1, 0, 0b1001);
    tss_e->segment = 0;     // 系统段
    tss_e->granularity = 0; // 字节
    tss_e->big = 0;         // 固定为 0
    asm volatile("ltr %w0" : : "r" (SELECTOR_TSS));
}

void update_tss_esp(struct task_struct* pthread)
{
    tss.esp0 = (uint32)((uint32)pthread + PAGE_SIZE);
}