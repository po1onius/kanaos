
#ifndef __THREAD_H__
#define __THREAD_H__

#include <types.h>
#include <list.h>
#include <memory.h>

#define STACK_MAGIC 0x20230207

#define TASK_FILE_NR 16 // 进程文件数量


/* 自定义通用函数类型,它将在很多线程函数中做为形参类型 */
typedef void thread_func(void*);

/* 进程或线程的状态 */
enum task_status
{
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/***********   中断栈intr_stack   ***********
 * 此结构用于中断发生时保护程序(线程或进程)的上下文环境:
 * 进程或线程被外部中断或软中断打断时,会按照此结构压入上下文
 * 寄存器,  intr_exit中的出栈操作是此结构的逆操作
 * 此栈在线程自己的内核栈中位置固定,所在页的最顶端
********************************************/
struct intr_stack
{
    uint32 vec_no;	 // kernel.S 宏VECTOR中push %1压入的中断号
    uint32 edi;
    uint32 esi;
    uint32 ebp;
    uint32 esp_dummy;	 // 虽然pushad把esp也压入,但esp是不断变化的,所以会被popad忽略
    uint32 ebx;
    uint32 edx;
    uint32 ecx;
    uint32 eax;
    uint32 gs;
    uint32 fs;
    uint32 es;
    uint32 ds;

/* 以下由cpu从低特权级进入高特权级时压入 */
    uint32 intr_vec;
    uint32 err_code;		 // err_code会被压入在eip之后
    void (*eip) (void);
    uint32 cs;
    uint32 eflags;
    void* esp;
    uint32 ss;
};

/***********  线程栈thread_stack  ***********
 * 线程自己的栈,用于存储线程中待执行的函数
 * 此结构在线程自己的内核栈中位置不固定,
 * 用在switch_to时保存线程环境。
 * 实际位置取决于实际运行情况。
 ******************************************/
struct thread_stack
{
    uint32 ebp;
    uint32 ebx;
    uint32 edi;
    uint32 esi;

/* 线程第一次执行时,eip指向待调用的函数kernel_thread
其它时候,eip是指向switch_to的返回地址*/
    void (*eip) (thread_func* func, void* func_arg);

/*****   以下仅供第一次被调度上cpu时使用   ****/

/* 参数unused_ret只为占位置充数为返回地址 */
    void (*unused_retaddr);
    thread_func* function;   // 由Kernel_thread所调用的函数名
    void* func_arg;    // 由Kernel_thread所调用的函数所需的参数
};

/* 进程或线程的pcb,程序控制块 */
struct task_struct
{
    uint32* self_kstack;	 // 各内核线程都用自己的内核栈
    uint32 pid;
    uint32 ppid;
    uint32 uid;
    uint32 gid;
    enum task_status status;
    uint8 priority;		 // 线程优先级
    int8 name[16];
    uint8 ticks;
    uint32 abs_ticks;
    struct list_node cur_list_tag;
    struct list_node all_list_tag;
    uint32 pgdir;
    struct vaddr_pool user_vaddr;
    struct mem_block_desc u_block_desc[DESC_CNT];
    struct inode_t *ipwd;     // 进程当前目录 inode program work directory
    struct inode_t *iroot;    // 进程根目录 inode
    uint16 umask;
    struct file_t *files[TASK_FILE_NR]; // 进程文件表
    int8 *pwd;
    uint32 stack_magic;	 // 用这串数字做栈的边界标记,用于检测栈的溢出
};

void thread_create(struct task_struct* pthread, thread_func function, void* func_arg);
void init_thread(struct task_struct* pthread, int8* name, uint8 prio);
struct task_struct* thread_start(int8* name, uint8 prio, thread_func function, void* func_arg);
void thread_init();
void schedule();
struct task_struct* running_thread();
void thread_unblock(struct task_struct* pthread);
void thread_block(enum task_status stat);
uint32 alloc_pid();

extern struct list thread_ready_list;
extern struct list thread_all_list;

int32 task_get_fd(struct task_struct *task);
void task_put_fd(struct task_struct *task, int32 fd);

#endif
