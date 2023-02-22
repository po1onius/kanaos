#include <device.h>
#include <string.h>
#include <thread.h>
#include <assert.h>
#include <debug.h>
#include <memory.h>
#include <syscall.h>
#include <list.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define DEVICE_NR 64 // 设备数量

static struct device_t devices[DEVICE_NR]; // 设备数组

// 获取空设备
static struct device_t *get_null_device()
{
    for (size_t i = 1; i < DEVICE_NR; i++)
    {
        struct device_t *device = &devices[i];
        if (device->type == DEV_NULL)
        {
            return device;
        }
    }
    panic("no more devices!!!");
}

int32 device_ioctl(int32 dev, int32 cmd, void *args, int32 flags)
{
    struct device_t *device = device_get(dev);
    if (device->ioctl)
    {
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    LOGK("ioctl of device %d not implemented!!!\n", dev);
    return EOF;
}

int32 device_read(int32 dev, void *buf, size_t count, uint32 idx, int32 flags)
{
    struct device_t *device = device_get(dev);
    if (device->read)
    {
        return device->read(device->ptr, buf, count, idx, flags);
    }
    LOGK("read of device %d not implemented!!!\n", dev);
    return EOF;
}

int32 device_write(int32 dev, void *buf, size_t count, uint32 idx, int32 flags)
{
    struct device_t *device = device_get(dev);
    if (device->write)
    {
        return device->write(device->ptr, buf, count, idx, flags);
    }
    LOGK("write of device %d not implemented!!!\n", dev);
    return EOF;
}

// 安装设备
int32 device_install(
        int32 type, int32 subtype,
        void *ptr, int8 *name, int32 parent,
        void *ioctl, void *read, void *write)
{
    struct device_t *device = get_null_device();
    device->ptr = ptr;
    device->parent = parent;
    device->type = type;
    device->subtype = subtype;
    strncpy(device->name, name, NAMELEN);
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;
    return device->dev;
}

void device_init()
{
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        struct device_t *device = &devices[i];
        strcpy((int8 *)device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = NULL;
        device->read = NULL;
        device->write = NULL;
        list_init(&device->request_list);
        device->direct = DIRECT_UP;
    }
}

struct device_t *device_find(int32 subtype, uint32 idx)
{
    uint32 nr = 0;
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        struct device_t *device = &devices[i];
        if (device->subtype != subtype)
        {
            continue;
        }
        if (nr == idx)
        {
            return device;
        }
        nr++;
    }
    return NULL;
}

struct device_t *device_get(int32 dev)
{
    assert(dev < DEVICE_NR);
    struct device_t *device = &devices[dev];
    assert(device->type != DEV_NULL);
    return device;
}


static void do_request(struct request_t *req)
{
    switch (req->type)
    {
        case REQ_READ:
            device_read(req->dev, req->buf, req->count, req->idx, req->flags);
            break;
        case REQ_WRITE:
            device_write(req->dev, req->buf, req->count, req->idx, req->flags);
            break;
        default:
            panic("req type %d unknown!!!");
            break;
    }
}

static struct request_t *request_nextreq(struct device_t *device, struct request_t *req)
{
    struct list *list = &device->request_list;

    if (device->direct == DIRECT_UP && req->node.next == &list->tail)
    {
        device->direct = DIRECT_DOWN;
    }
    else if (device->direct == DIRECT_DOWN && req->node.pre == &list->head)
    {
        device->direct = DIRECT_UP;
    }

    void *next = NULL;
    if (device->direct == DIRECT_UP)
    {
        next = req->node.next;
    }
    else
    {
        next = req->node.pre;
    }

    if (next == &list->head || next == &list->tail)
    {
        return NULL;
    }

    return elem2entry(struct request_t, node, next);
}

// 块设备请求
void device_request(int32 dev, void *buf, uint8 count, uint32 idx, int32 flags, uint32 type)
{
    struct device_t *device = device_get(dev);
    assert(device->type == DEV_BLOCK); // 是块设备
    uint32 offset = idx + device_ioctl(device->dev, DEV_CMD_SECTOR_START, 0, 0);

    if (device->parent)
    {
        device = device_get(device->parent);
    }

    struct request_t *req = malloc(sizeof(struct request_t));

    req->dev = device->dev;
    req->buf = buf;
    req->count = count;
    req->idx = offset;
    req->flags = flags;
    req->type = type;
    req->task = NULL;

    // 判断列表是否为空
    bool empty = list_empty(&device->request_list);

    // 将请求压入链表
    //list_push(&device->request_list, &req->node);
    list_insert_sort(&device->request_list, &req->node, element_node_offset(struct request_t, node, idx));
    // 如果列表不为空，则阻塞，因为已经有请求在处理了，等待处理完成；
    if (!empty)
    {
        req->task = running_thread();
        thread_block(TASK_BLOCKED);
    }

    do_request(req);
    struct request_t *nextreq = request_nextreq(device, req);

    list_remove(&req->node);
    free(req);

    if (nextreq)
    {
        thread_unblock(nextreq->task);
    }
}
