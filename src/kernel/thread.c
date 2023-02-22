#include <thread.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <intr.h>
#include <list.h>
#include <printk.h>
#include <process.h>
#include <debug.h>
#include <syscall.h>
#include <stdio.h>
#include <fs.h>

#define PG_SIZE 4096

static struct task_struct* main_thread;
struct list thread_ready_list;
struct list thread_all_list;
static struct list_node* thread_node;
static struct task_struct* idle_thread;

static struct lock pid_lock;

extern void task_switch(struct task_struct* next);


uint32 alloc_pid()
{
    static uint32 pidd = 0;
    lock_acquire(&pid_lock);
    pidd++;
    lock_release(&pid_lock);
    return pidd;
}

/* 由kernel_thread去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg)
{
    intr_enable();
    function(func_arg);
}

struct task_struct* running_thread()
{
    uint32 esp;
    asm ("mov %%esp, %0" : "=g" (esp));
    /* 取esp整数部分即pcb起始地址 */
    return (struct task_struct*)(esp & 0xfffff000);
}

/* 初始化线程栈thread_stack,将待执行的函数和参数放到thread_stack中相应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
    /* 先预留中断使用栈的空间,可见thread.h中定义的结构 */
    pthread->self_kstack -= sizeof(struct intr_stack);

    /* 再留出线程栈空间,可见thread.h中定义 */
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = kthread_stack->esi = kthread_stack->edi = 0;
}

/* 初始化线程基本信息 */
void init_thread(struct task_struct* pthread, int8* name, uint8 prio)
{
    memset(pthread, 0, sizeof(*pthread));
    strcpy(pthread->name, name);
    pthread->pid = alloc_pid();
    pthread->ppid = -1;
    pthread->gid = 0;
    pthread->umask = 0022;
    pthread->status = TASK_RUNNING;
    pthread->priority = prio;
    pthread->ticks = prio;
    pthread->abs_ticks = 0;
    pthread->pgdir = NULL;
/* self_kstack是线程自己在内核态下使用的栈顶地址 */
    pthread->self_kstack = (uint32*)((uint32)pthread + PG_SIZE);
    pthread->iroot = pthread->ipwd = get_root_inode();
    pthread->iroot->count += 2;
    pthread->pwd = (void *) get_kernel_pages(1);
    strcpy(pthread->pwd, "/");
    pthread->stack_magic = STACK_MAGIC;	  // 自定义的魔数
    if(pthread == main_thread)
    {
        pthread->status = TASK_RUNNING;
    }
    else
    {
        pthread->status = TASK_READY;
    }
}

/* 创建一优先级为prio的线程,线程名为name,线程所执行的函数是function(func_arg) */
struct task_struct* thread_start(int8* name, uint8 prio, thread_func function, void* func_arg)
{
/* pcb都位于内核空间,包括用户进程的pcb也是在内核空间 */
    struct task_struct* thread = get_kernel_pages(1);

    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    assert(!elem_find(&thread_ready_list, &thread->cur_list_tag));
    list_append_lock(&thread_ready_list, &thread->cur_list_tag);

    assert(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append_lock(&thread_all_list, &thread->all_list_tag);

    //asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret" : : "g" (thread->self_kstack) : "memory");
    return thread;
}

static void idle(void* arg)
{
    while(1)
    {
        thread_block(TASK_BLOCKED);
        //执行hlt时必须要保证目前处在开中断的情况下
        asm volatile ("sti; hlt" : : : "memory");
    }
}


void main_thread_init()
{
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);
    assert(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append_lock(&thread_all_list, &main_thread->all_list_tag);
}

void schedule()
{
    assert(intr_get_status() == INTR_OFF);
    struct task_struct* cur = running_thread();
    if(cur->status == TASK_RUNNING)
    {
        assert(!elem_find(&thread_ready_list, &cur->cur_list_tag));
        list_append_lock(&thread_ready_list, &cur->cur_list_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    }
    else
    {

    }
    if(list_empty(&thread_ready_list))
    {
        thread_unblock(idle_thread);
    }
    assert(!list_empty(&thread_ready_list));
    thread_node = NULL;
    thread_node = list_pop_lock(&thread_ready_list);
    struct task_struct* next = elem2entry(struct task_struct, cur_list_tag, thread_node);
    next->status = TASK_RUNNING;
    process_activate(next);
    task_switch(next);
}

void thread_init()
{
    lock_init(&pid_lock);
    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    main_thread_init();
    idle_thread = thread_start("idle", 31, idle, NULL);
}

/* 当前线程将自己阻塞,标志其状态为stat. */
void thread_block(enum task_status stat)
{
/* stat取值为TASK_BLOCKED,TASK_WAITING,TASK_HANGING,也就是只有这三种状态才不会被调度*/
    assert(((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING)));
    enum intr_status old_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat; // 置其状态为stat
    schedule();		      // 将当前线程换下处理器
/* 待当前线程被解除阻塞后才继续运行下面的intr_set_status */
    intr_set_status(old_status);
}

/* 将线程pthread解除阻塞 */
void thread_unblock(struct task_struct* pthread)
{
    enum intr_status old_status = intr_disable();
    assert(((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING)));
    if (pthread->status != TASK_READY)
    {
        assert(!elem_find(&thread_ready_list, &pthread->cur_list_tag));
        if (elem_find(&thread_ready_list, &pthread->cur_list_tag))
        {
            //PANIC("thread_unblock: blocked thread in ready_list\n");
        }
        list_push(&thread_ready_list, &pthread->cur_list_tag);    // 放到队列的最前面,使其尽快得到调度
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}

int32 task_get_fd(struct task_struct *task)
{
    int32 i;
    for (i = 3; i < TASK_FILE_NR; i++)
    {
        if (!task->files[i])
        {
            break;
        }
    }
    if (i == TASK_FILE_NR)
    {
        panic("Exceed task max open files.");
    }
    return i;
}

void task_put_fd(struct task_struct *task, int32 fd)
{
    if (fd < 3)
    {
        return;
    }
    assert(fd < TASK_FILE_NR);
    task->files[fd] = NULL;
}