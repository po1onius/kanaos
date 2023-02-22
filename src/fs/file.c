#include <fs.h>
#include <assert.h>
#include <thread.h>
#include <device.h>

#define FILE_NR 128

struct file_t file_table[FILE_NR];

// 从文件表中获取一个空文件
struct file_t *get_file()
{
    for (size_t i = 3; i < FILE_NR; i++)
    {
        struct file_t *file = &file_table[i];
        if (!file->count)
        {
            file->count++;
            return file;
        }
    }
    panic("Exceed max open files!!!");
}

void put_file(struct file_t *file)
{
    assert(file->count > 0);
    file->count--;
    if (!file->count)
    {
        iput(file->inode);
    }
}


int32 sys_open(int8 *filename, int32 flags, int32 mode)
{
    struct inode_t *inode = inode_open(filename, flags, mode);
    if (!inode)
    {
        return EOF;
    }

    struct task_struct *task = running_thread();
    int32 fd = task_get_fd(task);
    struct file_t *file = get_file();
    assert(task->files[fd] == NULL);
    task->files[fd] = file;

    file->inode = inode;
    file->flags = flags;
    file->count = 1;
    file->mode = inode->desc->mode;
    file->offset = 0;

    if (flags & O_APPEND)
    {
        file->offset = file->inode->desc->size;
    }
    return fd;
}

int32 sys_creat(int8 *filename, int32 mode)
{
    return sys_open(filename, O_CREAT | O_TRUNC, mode);
}

void sys_close(int32 fd)
{
    assert(fd < TASK_FILE_NR);
    struct task_struct *task = running_thread();
    struct file_t *file = task->files[fd];
    if (!file)
    {
        return;
    }

    assert(file->inode);
    put_file(file);
    task_put_fd(task, fd);
}

void file_init()
{
    for (size_t i = 0; i < FILE_NR; i++)
    {
        struct file_t *file = &file_table[i];
        file->mode = 0;
        file->count = 0;
        file->flags = 0;
        file->offset = 0;
        file->inode = NULL;
    }
}

int32 sys_read(int32 fd, int8 *buf, int32 count)
{
    if (fd == stdin)
    {
        struct device_t *device = device_find(DEV_KEYBOARD, 0);
        return device_read(device->dev, buf, count, 0, 0);
    }

    struct task_struct *task = running_thread();
    struct file_t *file = task->files[fd];
    assert(file);
    assert(count > 0);

    if ((file->flags & O_ACCMODE) == O_WRONLY)
    {
        return EOF;
    }

    struct inode_t *inode = file->inode;
    int32 len = inode_read(inode, buf, count, file->offset);
    if (len != EOF)
    {
        file->offset += len;
    }
    return len;
}

int32 sys_write(uint32 fd, int8 *buf, int32 count)
{
    if (fd == stdout || fd == stderr)
    {
        struct device_t *device = device_find(DEV_CONSOLE, 0);
        return device_write(device->dev, buf, count, 0, 0);
    }

    struct task_struct *task = running_thread();
    struct file_t *file = task->files[fd];
    assert(file);
    assert(count > 0);

    if ((file->flags & O_ACCMODE) == O_RDONLY)
    {
        return EOF;
    }

    struct inode_t *inode = file->inode;
    int32 len = inode_write(inode, buf, count, file->offset);
    if (len != EOF)
    {
        file->offset += len;
    }

    return len;
}
int32 sys_lseek(int32 fd, int32 offset, enum whence_t whence)
{
    assert(fd < TASK_FILE_NR);

    struct task_struct *task = running_thread();
    struct file_t *file = task->files[fd];

    assert(file);
    assert(file->inode);

    switch (whence)
    {
        case SEEK_SET:
            assert(offset >= 0);
            file->offset = offset;
            break;
        case SEEK_CUR:
            assert(file->offset + offset >= 0);
            file->offset += offset;
            break;
        case SEEK_END:
            assert(file->inode->desc->size + offset >= 0);
            file->offset = file->inode->desc->size + offset;
            break;
        default:
            panic("whence not defined !!!");
            break;
    }
    return file->offset;
}

int32 sys_readdir(int32 fd, struct dentry_t *dir, uint32 count)
{
    return sys_read(fd, (int8 *)dir, sizeof(struct dentry_t));
}