#include <types.h>
#include <thread.h>

uint16 sys_umask(uint16 mask)
{
    struct task_struct* task = running_thread();
    uint16 old_mask = task->umask;
    task->umask = mask & 0777;
    return old_mask;
}