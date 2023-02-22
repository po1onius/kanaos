#ifndef __GDT_H__
#define __GDT_H__

#include <types.h>

#define GDT_SIZE 128
extern struct dt_ptr gdtr;


#define	 RPL0  0
#define	 RPL1  1
#define	 RPL2  2
#define	 RPL3  3

#define TI_GDT 0
#define TI_LDT 1

#define KERNEL_CODE_IDX 1
#define KERNEL_DATA_IDX 2
#define USER_CODE_IDX 4
#define USER_DATA_IDX 5


#define SELECTOR_K_CODE	   ((KERNEL_CODE_IDX << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_K_DATA	   ((KERNEL_DATA_IDX << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_U_CODE	   ((USER_CODE_IDX << 3) + (TI_GDT << 2) + RPL3)
#define SELECTOR_U_DATA	   ((USER_DATA_IDX << 3) + (TI_GDT << 2) + RPL3)

struct descriptor
{
    uint16 limit_low;      // 段界限 0 ~ 15 位
    uint32 base_low : 24;    // 基地址 0 ~ 23 位 16M
    uint8 type : 4;        // 段类型
    uint8 segment : 1;     // 1 表示代码段或数据段，0 表示系统段
    uint8 DPL : 2;         // Descriptor Privilege Level 描述符特权等级 0 ~ 3
    uint8 present : 1;     // 存在位，1 在内存中，0 在磁盘上
    uint8 limit_high : 4;  // 段界限 16 ~ 19;
    uint8 available : 1;   // 该安排的都安排了，送给操作系统吧
    uint8 long_mode : 1;   // 64 位扩展标志
    uint8 big : 1;         // 32 位 还是 16 位;
    uint8 granularity : 1; // 粒度 4KB 或 1B
    uint8 base_high;       // 基地址 24 ~ 31 位
}_packed;

extern struct descriptor gdt[GDT_SIZE];

void gdt_init();
void make_gdt_desc(struct descriptor* desc, uint32 base, uint32 limit, uint8 dpl, uint8 type);


#endif