#ifndef __IDE_H__
#define __IDE_H__

#include <types.h>
#include <lock.h>

#define SECTOR_SIZE 512 // 扇区大小

#define IDE_CTRL_NR 2 // 控制器数量，固定为 2
#define IDE_DISK_NR 2 // 每个控制器可挂磁盘数量，固定为 2
#define IDE_PART_NR 4 // 每个磁盘分区数量，只支持主分区，总共 4 个

struct part_entry_t
{
    uint8 bootable;             // 引导标志
    uint8 start_head;           // 分区起始磁头号
    uint8 start_sector : 6;     // 分区起始扇区号
    uint16 start_cylinder : 10; // 分区起始柱面号
    uint8 system;               // 分区类型字节
    uint8 end_head;             // 分区的结束磁头号
    uint8 end_sector : 6;       // 分区结束扇区号
    uint16 end_cylinder : 10;   // 分区结束柱面号
    uint32 start;               // 分区起始物理扇区号 LBA
    uint32 count;               // 分区占用的扇区数
} _packed;

struct boot_sector_t
{
    uint8 code[446];
    struct part_entry_t entry[4];
    uint16 signature;
} _packed;

struct ide_part_t
{
    int8 name[8];            // 分区名称
    struct ide_disk_t *disk; // 磁盘指针
    uint32 system;              // 分区类型
    uint32 start;               // 分区起始物理扇区号 LBA
    uint32 count;               // 分区占用的扇区数
};

// IDE 磁盘
struct ide_disk_t
{
    int8 name[8];                  // 磁盘名称
    struct ide_ctrl_t *ctrl;       // 控制器指针
    uint8 selector;                   // 磁盘选择
    bool master;                   // 主盘
    uint32 total_lba;                 // 可用扇区数量
    uint32 cylinders;                 // 柱面数
    uint32 heads;                     // 磁头数
    uint32 sectors;                   // 扇区数
    struct ide_part_t parts[IDE_PART_NR]; // 硬盘分区
};

// IDE 控制器
struct ide_ctrl_t
{
    int8 name[8];                  // 控制器名称
    struct lock lock;                   // 控制器锁
    uint16 iobase;                    // IO 寄存器基址
    struct ide_disk_t disks[IDE_DISK_NR]; // 磁盘
    struct ide_disk_t *active;            // 当前选择的磁盘
    uint8 control;                    // 控制字节
    struct task_struct *waiter;         // 等待控制器的进程
};

int32 ide_pio_read(struct ide_disk_t *disk, void *buf, uint8 count, uint32 lba);
int32 ide_pio_write(struct ide_disk_t *disk, void *buf, uint8 count, uint32 lba);
void ide_handler1();
void ide_handler2();
void ide_init();
void pio_test();

#endif