#ifndef __INTR_H__
#define __INTR_H__

#include <types.h>

#define IDT_SIZE 256


struct idt
{
    uint16 offset_l;//中断调用例程(函数)地址低16位
    uint16 selector;//段选择子,用来索引gdt,表示中断调用例程所在的段
    uint8 reverse;//保留
    uint8 type : 4;//中断门/陷阱门/任务门...
    uint8 segment : 1;//是否是系统段
    uint8 DPL : 2;//中断门特权级
    uint8 present : 1;//是否有效
    uint16 offset_h;//中断调用例程(函数)地址高16位
}_packed;

void intr_init();

enum intr_status // 中断状态
{
    INTR_OFF,			 // 中断关闭
    INTR_ON		         // 中断打开
};

enum intr_status intr_get_status();
enum intr_status intr_set_status (enum intr_status status);
enum intr_status intr_enable ();
enum intr_status intr_disable ();
void send_eoi(uint32 vector);


#define INTR_ENTRY_SIZE 0x30

#endif