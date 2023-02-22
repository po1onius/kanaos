#include <intr.h>
#include <printk.h>
#include <clock.h>
#include <rtc.h>
#include <io.h>
#include <keyboard.h>
#include <debug.h>
#include <ide.h>

static struct idt idt[IDT_SIZE];
static struct dt_ptr idtr;

extern uint32 handler_table[INTR_ENTRY_SIZE];
extern void syscall_handler();


#define EFLAGS_IF   0x00000200       // eflags寄存器中的if位为1
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))


#define PIC_M_CTRL 0x20 // 主片的控制端口
#define PIC_M_DATA 0x21 // 主片的数据端口
#define PIC_S_CTRL 0xa0 // 从片的控制端口
#define PIC_S_DATA 0xa1 // 从片的数据端口
#define PIC_EOI 0x20    // 通知中断控制器中断结束


static char* handler_messages[] = {
    "#DE Divide Error\0",
    "#DB RESERVED\0",
    "--  NMI Interrupt\0",
    "#BP Breakpoint\0",
    "#OF Overflow\0",
    "#BR BOUND Range Exceeded\0",
    "#UD Invalid Opcode (Undefined Opcode)\0",
    "#NM Device Not Available (No Math Coprocessor)\0",
    "#DF Double Fault\0",
    "    Coprocessor Segment Overrun (reserved)\0",
    "#TS Invalid TSS\0",
    "#NP Segment Not Present\0",
    "#SS Stack-Segment Fault\0",
    "#GP General Protection\0",
    "#PF Page Fault\0",
    "--  (Intel reserved. Do not use.)\0",
    "#MF x87 FPU Floating-Point Error (Math Fault)\0",
    "#AC Alignment Check\0",
    "#MC Machine Check\0",
    "#XF SIMD Floating-Point Exception\0",
    "#VE Virtualization Exception\0",
    "#CP Control Protection Exception\0",
};

//告知外中断控制器,中断处理完毕 
void send_eoi(uint32 vector)
{
    if (vector >= 0x20 && vector < 0x28)
    {
        outb(PIC_M_CTRL, PIC_EOI);
    }
    if (vector >= 0x28 && vector < 0x30)
    {
        outb(PIC_M_CTRL, PIC_EOI);
        outb(PIC_S_CTRL, PIC_EOI);
    }
}


// 初始化外中断控制器
static void pic_init()
{
    outb(PIC_M_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_M_DATA, 0x20);       // ICW2: 起始端口号 0x20
    outb(PIC_M_DATA, 0b00000100); // ICW3: IR2接从片.
    outb(PIC_M_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    outb(PIC_S_CTRL, 0b00010001); // ICW1: 边沿触发, 级联 8259, 需要ICW4.
    outb(PIC_S_DATA, 0x28);       // ICW2: 起始端口号 0x28
    outb(PIC_S_DATA, 2);          // ICW3: 设置从片连接到主片的 IR2 引脚
    outb(PIC_S_DATA, 0b00000001); // ICW4: 8086模式, 正常EOI

    // 关闭主片外中断
    //8bit: 并口1 | 软盘 | 并口2 | 串口1 | 串口2 | 级联 | 键盘 | 时钟
    outb(PIC_M_DATA, 0b11111000);

    // 关闭从片外中断
    //8bit: 保留 | 硬盘 | 协处理器 | PS2鼠标 | 保留 | 保留 | 级联 | RTC实时时钟
    outb(PIC_S_DATA, 0b00111110);
}

//中断例程函数
void handler_warp(uint32 vec)
{
    if (vec < 0x16)//异常
    {
        printk("%d:%s\n", vec, handler_messages[vec]);
    }
    else if(vec >= 0x16 && vec < 0x20)//未定义
    {
        printk("%d:reserved exception\n", vec);
    }
    else if(vec == 0x20)//时钟
    {
        send_eoi(vec);
        clock_intr_handler();
    }
    else if(vec == 0x21)
    {
        send_eoi(vec);
        keyboard_handler();
    }
    else if(vec == 0x28)//rtc
    {
        send_eoi(vec);
        rtc_intr_handler();
    }
    else if(vec == 0x2e)
    {
        send_eoi(vec);
        ide_handler1();
    }
    else if(vec == 0x2f)
    {
        send_eoi(vec);
        ide_handler2();
    }
    else//目前只定义了0x30下的中断函数
    {
        //todo
        printk("%d undefine\n");
    }
}


static void idt_init()
{
    for(size_t i = 0; i < INTR_ENTRY_SIZE; ++i)
    {
        struct idt* t = (idt + i);
        t->offset_l = handler_table[i] & 0xffff;
        t->offset_h = (handler_table[i] >> 16) & 0xffff;
        t->present = 1;
        t->selector = 1 << 3;
        t->type = 0b1110;
        t->DPL = 0;
        t->reverse = 0;
        t->segment =  0;
    }

    struct idt *gate = &idt[0x80];
    gate->offset_l = (uint32)syscall_handler & 0xffff;
    gate->offset_h = ((uint32)syscall_handler >> 16) & 0xffff;
    gate->selector = 1 << 3; // 代码段
    gate->reverse = 0;      // 保留不用
    gate->type = 0b1110;     // 中断门
    gate->segment = 0;       // 系统段
    gate->DPL = 3;           // 用户态
    gate->present = 1;       // 有效

    idtr.base = (uint32)idt;
    idtr.limit = sizeof(idt) - 1;
    asm volatile("lidt idtr\n");
}


void intr_init()
{
    pic_init();
    idt_init();
}



/* 开中断并返回开中断前的状态*/
enum intr_status intr_enable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        asm volatile("sti");	 // 开中断,sti指令将IF位置1
        return old_status;
    }
}

/* 关中断,并且返回关中断前的状态 */
enum intr_status intr_disable()
{
    enum intr_status old_status;
    if (INTR_ON == intr_get_status())
    {
        old_status = INTR_ON;
        asm volatile("cli" : : : "memory"); // 关中断,cli指令将IF位置0
        return old_status;
    }
    else
    {
        old_status = INTR_OFF;
        return old_status;
    }
}

/* 将中断状态设置为status */
enum intr_status intr_set_status(enum intr_status status)
{
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/* 获取当前中断状态 */
enum intr_status intr_get_status()
{
    uint32 eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}