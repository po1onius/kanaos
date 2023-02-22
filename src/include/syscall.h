#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#define SYSCALL_SIZE 64

#include <types.h>

enum syscall_n
{
    SYS_NR_TEST,
    SYS_NR_YIELD,
    SYS_NR_GETPID,
    SYS_NR_MALLOC,
    SYS_NR_FREE,
    SYS_NR_UMASK,
    SYS_NR_TIME,
    SYS_NR_MKDIR,
    SYS_NR_RMDIR,
    SYS_NR_LINK,
    SYS_NR_UNLINK,
    SYS_NR_OPEN,
    SYS_NR_CREATE,
    SYS_NR_CLOSE,
    SYS_NR_READ,
    SYS_NR_WRITE,
    SYS_NR_LSEEK,
    SYS_NR_CHROOT,
    SYS_NR_CHDIR,
    SYS_NR_GETCWD,
    SYS_NR_FORK,
    SYS_NR_CLEAR,
    SYS_NR_READDIR
};

uint32 test();
void yield();
uint32 getpid();
void* malloc(uint32 size);
void free(void* addr);
uint16 umask(uint16 mask);
time_t time();
int32 mkdir(int8 *pathname, int32 mode);
int32 rmdir(int8 *pathname);
int32 link(int8 *oldname, int8 *newname);
int32 unlink(int8 *filename);
int32 open(int8 *filename, int32 flags, int32 mode);
int32 creat(int8 *filename, int32 mode);
void close(int32 fd);
int32 write(int32 fd, int8 *buf, int32 len);
int32 read(int32 fd, int8 *buf, uint32 len);
int32 lseek(int32 fd, int32 offset, int32 whence);
int8 *getcwd(int8 *buf, size_t size);
int32 chdir(int8 *pathname);
int32 chroot(int8 *pathname);
int32 fork();
void clear();
int32 readdir(int32 fd, void *dir, int32 count);

void syscall_init();

#endif
