[bits 32]
; 中断处理函数入口 


extern handler_warp

%define ERROR_CODE nop
%define ZERO push 0

%macro INTERRUPT_HANDLER 2
section .text
interrupt_handler_%1:
%2;如果有error code,cpu会自动压入error code,没有的话,手动压入一个值,统一栈帧
push %1; 压入中断向量，跳转到中断入口
jmp interrupt_entry
%endmacro

interrupt_entry:
    push ds
    push es
    push fs
    push gs
    pusha
    mov eax, [esp + 12 * 4]
    push eax
    ; 调用中断处理函数，handler_table 中存储了中断处理函数的指针
    call handler_warp

global interrupt_exit
interrupt_exit:
    add esp, 4

    popa
    pop gs
    pop fs
    pop es
    pop ds
    ; 对应 push %1，调用结束恢复栈
    add esp, 8
    iret


;异常
INTERRUPT_HANDLER 0x00, ZERO; divide by zero
INTERRUPT_HANDLER 0x01, ZERO; debug
INTERRUPT_HANDLER 0x02, ZERO; non maskable interrupt
INTERRUPT_HANDLER 0x03, ZERO; breakpoint

INTERRUPT_HANDLER 0x04, ZERO; overflow
INTERRUPT_HANDLER 0x05, ZERO; bound range exceeded
INTERRUPT_HANDLER 0x06, ZERO; invalid opcode
INTERRUPT_HANDLER 0x07, ZERO; device not avilable

INTERRUPT_HANDLER 0x08, ERROR_CODE; double fault
INTERRUPT_HANDLER 0x09, ZERO; coprocessor segment overrun
INTERRUPT_HANDLER 0x0a, ERROR_CODE; invalid TSS
INTERRUPT_HANDLER 0x0b, ERROR_CODE; segment not present

INTERRUPT_HANDLER 0x0c, ERROR_CODE; stack segment fault
INTERRUPT_HANDLER 0x0d, ERROR_CODE; general protection fault
INTERRUPT_HANDLER 0x0e, ERROR_CODE; page fault
INTERRUPT_HANDLER 0x0f, ZERO; reserved

INTERRUPT_HANDLER 0x10, ZERO; x87 floating point exception
INTERRUPT_HANDLER 0x11, ERROR_CODE; alignment check
INTERRUPT_HANDLER 0x12, ZERO; machine check
INTERRUPT_HANDLER 0x13, ZERO; SIMD Floating - Point Exception

INTERRUPT_HANDLER 0x14, ZERO; Virtualization Exception
INTERRUPT_HANDLER 0x15, ERROR_CODE; Control Protection Exception
INTERRUPT_HANDLER 0x16, ZERO; reserved
INTERRUPT_HANDLER 0x17, ZERO; reserved

INTERRUPT_HANDLER 0x18, ZERO; reserved
INTERRUPT_HANDLER 0x19, ZERO; reserved
INTERRUPT_HANDLER 0x1a, ZERO; reserved
INTERRUPT_HANDLER 0x1b, ZERO; reserved

INTERRUPT_HANDLER 0x1c, ZERO; reserved
INTERRUPT_HANDLER 0x1d, ZERO; reserved
INTERRUPT_HANDLER 0x1e, ZERO; reserved
INTERRUPT_HANDLER 0x1f, ZERO; reserved


;外中断
INTERRUPT_HANDLER 0x20, ZERO; clock 时钟中断
INTERRUPT_HANDLER 0x21, ZERO
INTERRUPT_HANDLER 0x22, ZERO
INTERRUPT_HANDLER 0x23, ZERO
INTERRUPT_HANDLER 0x24, ZERO
INTERRUPT_HANDLER 0x25, ZERO
INTERRUPT_HANDLER 0x26, ZERO
INTERRUPT_HANDLER 0x27, ZERO
INTERRUPT_HANDLER 0x28, ZERO
INTERRUPT_HANDLER 0x29, ZERO
INTERRUPT_HANDLER 0x2a, ZERO
INTERRUPT_HANDLER 0x2b, ZERO
INTERRUPT_HANDLER 0x2c, ZERO
INTERRUPT_HANDLER 0x2d, ZERO
INTERRUPT_HANDLER 0x2e, ZERO
INTERRUPT_HANDLER 0x2f, ZERO


section .data
global handler_table
handler_table:
dd interrupt_handler_0x00
dd interrupt_handler_0x01
dd interrupt_handler_0x02
dd interrupt_handler_0x03
dd interrupt_handler_0x04
dd interrupt_handler_0x05
dd interrupt_handler_0x06
dd interrupt_handler_0x07
dd interrupt_handler_0x08
dd interrupt_handler_0x09
dd interrupt_handler_0x0a
dd interrupt_handler_0x0b
dd interrupt_handler_0x0c
dd interrupt_handler_0x0d
dd interrupt_handler_0x0e
dd interrupt_handler_0x0f
dd interrupt_handler_0x10
dd interrupt_handler_0x11
dd interrupt_handler_0x12
dd interrupt_handler_0x13
dd interrupt_handler_0x14
dd interrupt_handler_0x15
dd interrupt_handler_0x16
dd interrupt_handler_0x17
dd interrupt_handler_0x18
dd interrupt_handler_0x19
dd interrupt_handler_0x1a
dd interrupt_handler_0x1b
dd interrupt_handler_0x1c
dd interrupt_handler_0x1d
dd interrupt_handler_0x1e
dd interrupt_handler_0x1f
dd interrupt_handler_0x20
dd interrupt_handler_0x21
dd interrupt_handler_0x22
dd interrupt_handler_0x23
dd interrupt_handler_0x24
dd interrupt_handler_0x25
dd interrupt_handler_0x26
dd interrupt_handler_0x27
dd interrupt_handler_0x28
dd interrupt_handler_0x29
dd interrupt_handler_0x2a
dd interrupt_handler_0x2b
dd interrupt_handler_0x2c
dd interrupt_handler_0x2d
dd interrupt_handler_0x2e
dd interrupt_handler_0x2f

extern syscall_check
extern syscall_table
section .text
global syscall_handler
syscall_handler:
    push eax
    call syscall_check
    add esp, 4

    push 0x20230207

    push 0x80

    ; 保存上文寄存器信息
    push ds
    push es
    push fs
    push gs
    pusha

    push 0x80; 向中断处理函数传递参数中断向量 vector

    push ebp; 第六个参数
    push edi; 第五个参数
    push esi; 第四个参数
    push edx; 第三个参数
    push ecx; 第二个参数
    push ebx; 第一个参数

    ; 调用系统调用处理函数，syscall_table 中存储了系统调用处理函数的指针
    call [syscall_table + eax * 4]


    add esp, (6 * 4); 系统调用结束恢复栈

    ; 修改栈中 eax 寄存器，设置系统调用返回值
    mov dword [esp + 8 * 4], eax

    ; 跳转到中断返回
    jmp interrupt_exit