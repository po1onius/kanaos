#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <types.h>
#include <thread.h>

#define default_prio 31
#define USER_STACK3_VADDR  (0xc0000000 - 0x1000)
#define USER_VADDR_START 0x8048000
#define EFLAGS_MBS	(1 << 1)	// 此项必须要设置
#define EFLAGS_IF_1	(1 << 9)	// if为1,开中断
#define EFLAGS_IF_0	0		// if为0,关中断
#define EFLAGS_IOPL_3	(3 << 12)	// IOPL3,用于测试用户程序在非系统调用下进行IO
#define EFLAGS_IOPL_0	(0 << 12)	// IOPL0
#define DIV_ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))


void process_execute(void* filename, int8* name);
void start_process(void* filename_);
void process_activate(struct task_struct* p_thread);
void page_dir_activate(struct task_struct* p_thread);
uint32* create_page_dir(void);
void create_user_vaddr_bitmap(struct task_struct* user_prog);

#endif
