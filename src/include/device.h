#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <types.h>
#include <list.h>
#include <thread.h>
#include <list.h>

#define NAMELEN 16

// 设备类型
enum device_type_t
{
    DEV_NULL,  // 空设备
    DEV_CHAR,  // 字符设备
    DEV_BLOCK, // 块设备
};

// 设备子类型
enum device_subtype_t
{
    DEV_CONSOLE = 1, // 控制台
    DEV_KEYBOARD,    // 键盘
    DEV_SERIAL,      // 串口
    DEV_IDE_DISK,    // IDE 磁盘
    DEV_IDE_PART,    // IDE 磁盘分区
    DEV_RAMDISK,     // 虚拟磁盘
};

// 设备控制命令
enum device_cmd_t
{
    DEV_CMD_SECTOR_START = 1, // 获得设备扇区开始位置 lba
    DEV_CMD_SECTOR_COUNT,     // 获得设备扇区数量
};

#define REQ_READ 0  // 块设备读
#define REQ_WRITE 1 // 块设备写

#define DIRECT_UP 0   // 上楼
#define DIRECT_DOWN 1 // 下楼

// 块设备请求
struct request_t
{
    int32 dev;           // 设备号
    uint32 type;            // 请求类型
    uint32 idx;             // 扇区位置
    uint32 count;           // 扇区数量
    int32 flags;           // 特殊标志
    uint8 *buf;             // 缓冲区
    struct task_struct *task; // 请求进程
    struct list_node node;    // 列表节点
};

struct device_t
{
    int8 name[NAMELEN];  // 设备名
    int32 type;            // 设备类型
    int32 subtype;         // 设备子类型
    int32 dev;           // 设备号
    int32 parent;        // 父设备号
    void *ptr;           // 设备指针
    struct list request_list; // 块设备请求链表
    bool direct;         // 磁盘寻道方向

    // 设备控制
    int32 (*ioctl)(void *dev, int32 cmd, void *args, int32 flags);
    // 读设备
    int32 (*read)(void *dev, void *buf, size_t count, uint32 idx, int32 flags);
    // 写设备
    int32 (*write)(void *dev, void *buf, size_t count, uint32 idx, int32 flags);
};

// 安装设备
int32 device_install(
        int32 type, int32 subtype,
        void *ptr, int8 *name, int32 parent,
        void *ioctl, void *read, void *write);

// 根据子类型查找设备
struct device_t *device_find(int32 type, uint32 idx);

// 根据设备号查找设备
struct device_t *device_get(int32 dev);

// 控制设备
int32 device_ioctl(int32 dev, int32 cmd, void *args, int32 flags);

// 读设备
int32 device_read(int32 dev, void *buf, size_t count, uint32 idx, int32 flags);

// 写设备
int32 device_write(int32 dev, void *buf, size_t count, uint32 idx, int32 flags);

// 块设备请求
void device_request(int32 dev, void *buf, uint8 count, uint32 idx, int32 flags, uint32 type);

#endif