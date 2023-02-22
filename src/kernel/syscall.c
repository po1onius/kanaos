#include <syscall.h>
#include <types.h>
#include <printk.h>
#include <thread.h>
#include <stdio.h>
#include <debug.h>
#include <fs.h>


uint32 syscall_table[SYSCALL_SIZE];

void syscall_check(uint32 n)
{
    if(n > SYSCALL_SIZE)
    {

    }
}

/* 无参数的系统调用 */
#define _syscall0(NUMBER) ({				       \
   int32 retval;					               \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (retval)					       \
   : "a" (NUMBER)					       \
   : "memory"						       \
   );							       \
   retval;						       \
})

/* 一个参数的系统调用 */
#define _syscall1(NUMBER, ARG1) ({			       \
   int32 retval;					               \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (retval)					       \
   : "a" (NUMBER), "b" (ARG1)				       \
   : "memory"						       \
   );							       \
   retval;						       \
})

/* 两个参数的系统调用 */
#define _syscall2(NUMBER, ARG1, ARG2) ({		       \
   int32 retval;						       \
   asm volatile (					       \
   "int $0x80"						       \
   : "=a" (retval)					       \
   : "a" (NUMBER), "b" (ARG1), "c" (ARG2)		       \
   : "memory"						       \
   );							       \
   retval;						       \
})

/* 三个参数的系统调用 */
#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({		       \
   int32 retval;						       \
   asm volatile (					       \
      "int $0x80"					       \
      : "=a" (retval)					       \
      : "a" (NUMBER), "b" (ARG1), "c" (ARG2), "d" (ARG3)       \
      : "memory"					       \
   );							       \
   retval;						       \
})



void sys_default()
{

}

void sys_test()
{
    printk("test");
}

void sys_yield()
{
    schedule();
}

uint32 sys_getpid()
{
    return running_thread()->pid;
}

extern void* sys_malloc(uint32 size);
extern uint16 sys_umask(uint16 mask);
extern time_t sys_time();
extern int32 sys_mkdir(int8 *pathname, int32 mode);
extern int32 sys_rmdir(int8 *pathname);
extern int32 sys_link(int8 *oldname, int8 *newname);
extern int32 sys_unlink(int8 *filename);
extern int32 sys_open(int8 *filename, int32 flags, int32 mode);
extern int32 sys_creat(int8 *filename, int32 mode);
extern void sys_close(int32 fd);
extern int32 sys_read(int32 fd, int8 *buf, int32 count);
extern int32 sys_write(uint32 fd, int8 *buf, int32 count);
extern int32 sys_lseek(int32 fd, int32 offset, int32 whence);
extern int8 *sys_getcwd(int8 *buf, size_t size);
extern int32 sys_chdir(int8 *pathname);
extern int32 sys_chroot(int8 *pathname);
extern int32 sys_fork();
extern void console_clear();
extern int32 sys_readdir(int32 fd, struct dentry_t *dir, uint32 count);



void syscall_init()
{
    for(size_t i = 0; i < SYSCALL_SIZE; i++)
    {
        syscall_table[i] = (uint32)sys_default;
    }
    syscall_table[SYS_NR_TEST] = (uint32)sys_test;
    syscall_table[SYS_NR_YIELD] = (uint32)sys_yield;
    syscall_table[SYS_NR_GETPID] = (uint32)sys_getpid;
    syscall_table[SYS_NR_MALLOC] = (uint32)sys_malloc;
    syscall_table[SYS_NR_FREE] = (uint32)sys_free;
    syscall_table[SYS_NR_UMASK] = (uint32) sys_umask;
    syscall_table[SYS_NR_TIME] = (uint32)sys_time;
    syscall_table[SYS_NR_MKDIR] = (uint32) sys_mkdir;
    syscall_table[SYS_NR_RMDIR] = (uint32) sys_rmdir;
    syscall_table[SYS_NR_LINK] = (uint32) sys_link;
    syscall_table[SYS_NR_UNLINK] = (uint32) sys_unlink;
    syscall_table[SYS_NR_OPEN] = (uint32) sys_open;
    syscall_table[SYS_NR_CREATE] = (uint32) sys_creat;
    syscall_table[SYS_NR_CLOSE] = (uint32) sys_close;
    syscall_table[SYS_NR_READ] = (uint32) sys_read;
    syscall_table[SYS_NR_WRITE] = (uint32) sys_write;
    syscall_table[SYS_NR_LSEEK] = (uint32) sys_lseek;
    syscall_table[SYS_NR_CHDIR] = (uint32) sys_chdir;
    syscall_table[SYS_NR_CHROOT] = (uint32) sys_chroot;
    syscall_table[SYS_NR_GETCWD] = (uint32) sys_getcwd;
    syscall_table[SYS_NR_FORK] = (uint32)sys_fork;
    syscall_table[SYS_NR_CLEAR] = (uint32)console_clear;
    syscall_table[SYS_NR_READDIR] = (uint32) sys_readdir;
}


uint32 test()
{
    return _syscall0(SYS_NR_TEST);
}

void yield()
{
    _syscall0(SYS_NR_YIELD);
}

uint32 getpid()
{
    _syscall0(SYS_NR_GETPID);
}

void* malloc(uint32 size)
{
    return (void*)_syscall1(SYS_NR_MALLOC, size);
}


void free(void* addr)
{
    _syscall1(SYS_NR_FREE, (uint32)addr);
}

uint16 umask(uint16 mask)
{
    return _syscall1(SYS_NR_UMASK, mask);
}

time_t time()
{
    return _syscall0(SYS_NR_TIME);
}
int32 mkdir(int8 *pathname, int32 mode)
{
    return _syscall2(SYS_NR_MKDIR, (uint32)pathname, (uint32)mode);
}

int32 rmdir(int8 *pathname)
{
    return _syscall1(SYS_NR_RMDIR, (uint32)pathname);
}

int32 link(int8 *oldname, int8 *newname)
{
    return _syscall2(SYS_NR_LINK, (uint32)oldname, (uint32)newname);
}

int32 unlink(int8 *filename)
{
    return _syscall1(SYS_NR_UNLINK, (uint32)filename);
}

int32 open(int8 *filename, int32 flags, int32 mode)
{
    return _syscall3(SYS_NR_OPEN, (uint32)filename, (uint32)flags, (uint32)mode);
}

int32 creat(int8 *filename, int32 mode)
{
    return _syscall2(SYS_NR_CREATE, (uint32)filename, (uint32)mode);
}

void close(int32 fd)
{
    _syscall1(SYS_NR_CLOSE, (uint32)fd);
}

int32 read(int32 fd, int8 *buf, uint32 len)
{
    return _syscall3(SYS_NR_READ, fd, (uint32)buf, len);
}

int32 write(int32 fd, int8 *buf, int32 len)
{
    return _syscall3(SYS_NR_WRITE, fd, (uint32)buf, len);
}

int32 lseek(int32 fd, int32 offset, int32 whence)
{
    return _syscall3(SYS_NR_LSEEK, fd, offset, whence);
}

int8 *getcwd(int8 *buf, size_t size)
{
    return (int8 *)_syscall2(SYS_NR_GETCWD, (uint32)buf, (uint32)size);
}

int32 chdir(int8 *pathname)
{
    return _syscall1(SYS_NR_CHDIR, (uint32)pathname);
}

int32 chroot(int8 *pathname)
{
    return _syscall1(SYS_NR_CHROOT, (uint32)pathname);
}

int32 fork()
{
    return _syscall0(SYS_NR_FORK);
}

void clear()
{
    _syscall0(SYS_NR_CLEAR);
}

int32 readdir(int32 fd, void *dir, int32 count)
{
    return _syscall3(SYS_NR_READDIR, fd, (uint32)dir, (uint32)count);
}