#include <ide.h>
#include <io.h>
#include <printk.h>
#include <stdio.h>
#include <memory.h>
#include <intr.h>
#include <thread.h>
#include <string.h>
#include <assert.h>
#include <debug.h>
#include <device.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// IDE 寄存器基址
#define IDE_IOBASE_PRIMARY 0x1F0   // 主通道基地址
#define IDE_IOBASE_SECONDARY 0x170 // 从通道基地址

// IDE 寄存器偏移
#define IDE_DATA 0x0000       // 数据寄存器
#define IDE_ERR 0x0001        // 错误寄存器
#define IDE_FEATURE 0x0001    // 功能寄存器
#define IDE_SECTOR 0x0002     // 扇区数量
#define IDE_LBA_LOW 0x0003    // LBA 低字节
#define IDE_LBA_MID 0x0004    // LBA 中字节
#define IDE_LBA_HIGH 0x0005   // LBA 高字节
#define IDE_HDDEVSEL 0x0006   // 磁盘选择寄存器
#define IDE_STATUS 0x0007     // 状态寄存器
#define IDE_COMMAND 0x0007    // 命令寄存器
#define IDE_ALT_STATUS 0x0206 // 备用状态寄存器
#define IDE_CONTROL 0x0206    // 设备控制寄存器
#define IDE_DEVCTRL 0x0206    // 驱动器地址寄存器

// IDE 命令

#define IDE_CMD_READ 0x20     // 读命令
#define IDE_CMD_WRITE 0x30    // 写命令
#define IDE_CMD_IDENTIFY 0xEC // 识别命令

// IDE 控制器状态寄存器
#define IDE_SR_NULL 0x00 // NULL
#define IDE_SR_ERR 0x01  // Error
#define IDE_SR_IDX 0x02  // Index
#define IDE_SR_CORR 0x04 // Corrected data
#define IDE_SR_DRQ 0x08  // Data request
#define IDE_SR_DSC 0x10  // Drive seek complete
#define IDE_SR_DWF 0x20  // Drive write fault
#define IDE_SR_DRDY 0x40 // Drive ready
#define IDE_SR_BSY 0x80  // Controller busy

// IDE 控制寄存器
#define IDE_CTRL_HD15 0x00 // Use 4 bits for head (not used, was 0x08)
#define IDE_CTRL_SRST 0x04 // Soft reset
#define IDE_CTRL_NIEN 0x02 // Disable interrupts

// IDE 错误寄存器
#define IDE_ER_AMNF 0x01  // Address mark not found
#define IDE_ER_TK0NF 0x02 // Track 0 not found
#define IDE_ER_ABRT 0x04  // Abort
#define IDE_ER_MCR 0x08   // Media change requested
#define IDE_ER_IDNF 0x10  // Sector id not found
#define IDE_ER_MC 0x20    // Media change
#define IDE_ER_UNC 0x40   // Uncorrectable data error
#define IDE_ER_BBK 0x80   // Bad block

#define IDE_LBA_MASTER 0b11100000 // 主盘 LBA
#define IDE_LBA_SLAVE 0b11110000  // 从盘 LBA


enum PART_FS
{
    PART_FS_FAT12 = 1,    // FAT12
    PART_FS_EXTENDED = 5, // 扩展分区
    PART_FS_MINIX = 0x80, // minux
    PART_FS_LINUX = 0x83, // linux
};

struct ide_params_t
{
    uint16 config;                 // 0 General configuration bits
    uint16 cylinders;              // 01 cylinders
    uint16 RESERVED;               // 02
    uint16 heads;                  // 03 heads
    uint16 RESERVED[5 - 3];        // 05
    uint16 sectors;                // 06 sectors per track
    uint16 RESERVED[9 - 6];        // 09
    uint8 serial[20];              // 10 ~ 19 序列号
    uint16 RESERVED[22 - 19];      // 10 ~ 22
    uint8 firmware[8];             // 23 ~ 26 固件版本
    uint8 model[40];               // 27 ~ 46 模型数
    uint8 drq_sectors;             // 47 扇区数量
    uint8 RESERVED[3];             // 48
    uint16 capabilities;           // 49 能力
    uint16 RESERVED[59 - 49];      // 50 ~ 59
    uint32 total_lba;              // 60 ~ 61
    uint16 RESERVED;               // 62
    uint16 mdma_mode;              // 63
    uint8 RESERVED;                // 64
    uint8 pio_mode;                // 64
    uint16 RESERVED[79 - 64];      // 65 ~ 79 参见 ATA specification
    uint16 major_version;          // 80 主版本
    uint16 minor_version;          // 81 副版本
    uint16 commmand_sets[87 - 81]; // 82 ~ 87 支持的命令集
    uint16 RESERVED[118 - 87];     // 88 ~ 118
    uint16 support_settings;       // 119
    uint16 enable_settings;        // 120
    uint16 RESERVED[221 - 120];    // 221
    uint16 transport_major;        // 222
    uint16 transport_minor;        // 223
    uint16 RESERVED[254 - 223];    // 254
    uint16 integrity;              // 校验和
} _packed;


struct ide_ctrl_t controllers[IDE_CTRL_NR];

// 硬盘中断处理函数
static void ide_handler(uint32 vector)
{
    // 得到中断向量对应的控制器
    struct ide_ctrl_t *ctrl = &controllers[vector - 0x2e];

    // 读取常规状态寄存器，表示中断处理结束
    uint8 state = inb(ctrl->iobase + IDE_STATUS);
    
    if (ctrl->waiter)
    {
        // 如果有进程阻塞，则取消阻塞
        thread_unblock(ctrl->waiter);
        ctrl->waiter = NULL;
    }
}

void ide_handler1()
{
    ide_handler(0x2e);
}

void ide_handler2()
{
    ide_handler(0x2f);
}

static uint32 ide_error(struct ide_ctrl_t *ctrl)
{
    uint8 error = inb(ctrl->iobase + IDE_ERR);
    if (error & IDE_ER_BBK)
    {
        LOGK("bad block\n");
    }
    if (error & IDE_ER_UNC)
    {
        LOGK("uncorrectable data\n");
    }
    if (error & IDE_ER_MC)
    {
        LOGK("media change\n");
    }
    if (error & IDE_ER_IDNF)
    {
        LOGK("id not found\n");
    }
    if (error & IDE_ER_MCR)
    {
        LOGK("media change requested\n");
    }
    if (error & IDE_ER_ABRT)
    {
        LOGK("abort\n");
    }
    if (error & IDE_ER_TK0NF)
    {
        LOGK("track 0 not found\n");
    }
    if (error & IDE_ER_AMNF)
    {
        LOGK("address mark not found\n");
    }
}

static uint32 ide_busy_wait(struct ide_ctrl_t *ctrl, uint8 mask)
{
    while (true)
    {
        // 从备用状态寄存器中读状态
        uint8 state = inb(ctrl->iobase + IDE_ALT_STATUS);
        if (state & IDE_SR_ERR) // 有错误
        {
            ide_error(ctrl);
        }
        if (state & IDE_SR_BSY) // 驱动器忙
        {
            continue;
        }
        if ((state & mask) == mask) // 等待的状态完成
        {
            return 0;
        }
    }
}

static void ide_reset_controller(struct ide_ctrl_t *ctrl)
{
    outb(ctrl->iobase + IDE_CONTROL, IDE_CTRL_SRST);
    ide_busy_wait(ctrl, IDE_SR_NULL);
    outb(ctrl->iobase + IDE_CONTROL, ctrl->control);
    ide_busy_wait(ctrl, IDE_SR_NULL);
}

// 选择磁盘
static void ide_select_drive(struct ide_disk_t *disk)
{
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, disk->selector);
    disk->ctrl->active = disk;
}

// 选择扇区
static void ide_select_sector(struct ide_disk_t *disk, uint32 lba, uint8 count)
{
    // 输出功能，可省略
    outb(disk->ctrl->iobase + IDE_FEATURE, 0);

    // 读写扇区数量
    outb(disk->ctrl->iobase + IDE_SECTOR, count);

    // LBA 低字节
    outb(disk->ctrl->iobase + IDE_LBA_LOW, lba & 0xff);
    // LBA 中字节
    outb(disk->ctrl->iobase + IDE_LBA_MID, (lba >> 8) & 0xff);
    // LBA 高字节
    outb(disk->ctrl->iobase + IDE_LBA_HIGH, (lba >> 16) & 0xff);

    // LBA 最高四位 + 磁盘选择
    outb(disk->ctrl->iobase + IDE_HDDEVSEL, ((lba >> 24) & 0xf) | disk->selector);
    disk->ctrl->active = disk;
}

// 从磁盘读取一个扇区到 buf
static void ide_pio_read_sector(struct ide_disk_t *disk, uint16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        buf[i] = inw(disk->ctrl->iobase + IDE_DATA);
    }
}

// 从 buf 写入一个扇区到磁盘
static void ide_pio_write_sector(struct ide_disk_t *disk, uint16 *buf)
{
    for (size_t i = 0; i < (SECTOR_SIZE / 2); i++)
    {
        outw(disk->ctrl->iobase + IDE_DATA, buf[i]);
    }
}

// PIO 方式读取磁盘
int32 ide_pio_read(struct ide_disk_t *disk, void *buf, uint8 count, uint32 lba)
{
    assert(count > 0);

    struct ide_ctrl_t *ctrl = disk->ctrl;
    enum intr_status old_status = intr_disable();
    lock_acquire(&ctrl->lock);
    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送读命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_READ);

    for (size_t i = 0; i < count; i++)
    {
        struct task_struct* task = running_thread();
        if(task->status == TASK_RUNNING)
        {
            ctrl->waiter = task;
            thread_block(TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_DRQ);
        uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
        ide_pio_read_sector(disk, (uint16 *)offset);
    }

    lock_release(&ctrl->lock);
    intr_set_status(old_status);
    return 0;
}

// PIO 方式写磁盘
int32 ide_pio_write(struct ide_disk_t *disk, void *buf, uint8 count, uint32 lba)
{
    assert(count > 0);
    enum intr_status old_status = intr_disable();
    struct ide_ctrl_t *ctrl = disk->ctrl;

    lock_acquire(&ctrl->lock);

    LOGK("write lba 0x%x\n", lba);

    // 选择磁盘
    ide_select_drive(disk);

    // 等待就绪
    ide_busy_wait(ctrl, IDE_SR_DRDY);

    // 选择扇区
    ide_select_sector(disk, lba, count);

    // 发送写命令
    outb(ctrl->iobase + IDE_COMMAND, IDE_CMD_WRITE);

    for (size_t i = 0; i < count; i++)
    {
        uint32 offset = ((uint32)buf + i * SECTOR_SIZE);
        ide_pio_write_sector(disk, (uint16 *)offset);

        struct task_struct* task = running_thread();
        if(task->status == TASK_RUNNING)
        {
            ctrl->waiter = task;
            thread_block(TASK_BLOCKED);
        }
        ide_busy_wait(ctrl, IDE_SR_NULL);
    }

    lock_release(&ctrl->lock);
    intr_set_status(old_status);
    return 0;
}

// 读分区
int32 ide_pio_part_read(struct ide_part_t *part, void *buf, uint8 count, uint32 lba)
{
    return ide_pio_read(part->disk, buf, count, part->start + lba);
}

// 写分区
int32 ide_pio_part_write(struct ide_part_t *part, void *buf, uint8 count, uint32 lba)
{
    return ide_pio_write(part->disk, buf, count, part->start + lba);
}

static void ide_swap_pairs(int8 *buf, uint32 len)
{
    for (size_t i = 0; i < len; i += 2)
    {
        register int8 ch = buf[i];
        buf[i] = buf[i + 1];
        buf[i + 1] = ch;
    }
    buf[len - 1] = '\0';
}

static uint32 ide_identify(struct ide_disk_t *disk, uint16 *buf)
{
    LOGK("identifing disk %s...\n", disk->name);
    lock_acquire(&disk->ctrl->lock);
    ide_select_drive(disk);

    // ide_select_sector(disk, 0, 0);

    outb(disk->ctrl->iobase + IDE_COMMAND, IDE_CMD_IDENTIFY);

    ide_busy_wait(disk->ctrl, IDE_SR_NULL);

    struct ide_params_t *params = (struct ide_params_t *)buf;

    ide_pio_read_sector(disk, buf);

    LOGK("disk %s total lba %d\n", disk->name, params->total_lba);

    uint32 ret = EOF;
    if (params->total_lba == 0)
    {
        goto rollback;
    }

    ide_swap_pairs(params->serial, sizeof(params->serial));
    LOGK("disk %s serial number %s\n", disk->name, params->serial);

    ide_swap_pairs(params->firmware, sizeof(params->firmware));
    LOGK("disk %s firmware version %s\n", disk->name, params->firmware);

    ide_swap_pairs(params->model, sizeof(params->model));
    LOGK("disk %s model number %s\n", disk->name, params->model);

    disk->total_lba = params->total_lba;
    disk->cylinders = params->cylinders;
    disk->heads = params->heads;
    disk->sectors = params->sectors;
    ret = 0;

    rollback:
    lock_release(&disk->ctrl->lock);
    return ret;
}

static void ide_part_init(struct ide_disk_t *disk, uint16 *buf)
{
    // 磁盘不可用
    if (!disk->total_lba)
    {
        return;
    }

    // 读取主引导扇区
    ide_pio_read(disk, buf, 1, 0);

    // 初始化主引导扇区
    struct boot_sector_t *boot = (struct boot_sector_t *)buf;

    for (size_t i = 0; i < IDE_PART_NR; i++)
    {
        struct part_entry_t *entry = &boot->entry[i];
        struct ide_part_t *part = &disk->parts[i];
        if (!entry->count)
        {
            continue;
        }

        sprintf(part->name, "%s%d", disk->name, i + 1);

        LOGK("part %s \n", part->name);
        LOGK("    bootable %d\n", entry->bootable);
        LOGK("    start %d\n", entry->start);
        LOGK("    count %d\n", entry->count);
        LOGK("    system 0x%x\n", entry->system);

        part->disk = disk;
        part->count = entry->count;
        part->system = entry->system;
        part->start = entry->start;

        if (entry->system == PART_FS_EXTENDED)
        {
            LOGK("Unsupported extended partition!!!\n");

            struct boot_sector_t *eboot = (struct boot_sector_t *)(buf + SECTOR_SIZE);
            ide_pio_read(disk, (void *)eboot, 1, entry->start);

            for (size_t j = 0; j < IDE_PART_NR; j++)
            {
                struct part_entry_t *eentry = &eboot->entry[j];
                if (!eentry->count)
                {
                    continue;
                }
                LOGK("part %d extend %d\n", i, j);
                LOGK("    bootable %d\n", eentry->bootable);
                LOGK("    start %d\n", eentry->start);
                LOGK("    count %d\n", eentry->count);
                LOGK("    system 0x%x\n", eentry->system);
            }
        }
    }
}

static void ide_ctrl_init()
{
    uint16 *buf = (uint16 *)get_kernel_pages(1);
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        struct ide_ctrl_t *ctrl = &controllers[cidx];
        sprintf(ctrl->name, "ide%u", cidx);
        lock_init(&ctrl->lock);
        ctrl->active = NULL;
        ctrl->waiter = NULL;
        if (cidx) // 从通道
        {
            ctrl->iobase = IDE_IOBASE_SECONDARY;
        }
        else // 主通道
        {
            ctrl->iobase = IDE_IOBASE_PRIMARY;
        }
        ctrl->control = inb(ctrl->iobase + IDE_CONTROL);
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            struct ide_disk_t *disk = &ctrl->disks[didx];
            sprintf(disk->name, "hd%c", 'a' + cidx * 2 + didx);
            disk->ctrl = ctrl;
            if (didx) // 从盘
            {
                disk->master = false;
                disk->selector = IDE_LBA_SLAVE;
            }
            else // 主盘
            {
                disk->master = true;
                disk->selector = IDE_LBA_MASTER;
            }
            ide_identify(disk, buf);
            ide_part_init(disk,buf);
        }
    }
    free_page(KERNEL_POOL, buf, 1);
}

int32 ide_pio_part_ioctl(struct ide_part_t *part, int32 cmd, void *args, int32 flags)
{
    switch (cmd)
    {
        case DEV_CMD_SECTOR_START:
            return part->start;
        case DEV_CMD_SECTOR_COUNT:
            return part->count;
        default:
            panic("device command %d can't recognize!!!", cmd);
            break;
    }
}

int32 ide_pio_ioctl(struct ide_disk_t *disk, int32 cmd, void *args, int32 flags)
{
    switch (cmd)
    {
        case DEV_CMD_SECTOR_START:
            return 0;
        case DEV_CMD_SECTOR_COUNT:
            return disk->total_lba;
        default:
            panic("device command %d can't recognize!!!", cmd);
            break;
    }
}

static void ide_install()
{
    for (size_t cidx = 0; cidx < IDE_CTRL_NR; cidx++)
    {
        struct ide_ctrl_t *ctrl = &controllers[cidx];
        for (size_t didx = 0; didx < IDE_DISK_NR; didx++)
        {
            struct ide_disk_t *disk = &ctrl->disks[didx];
            if (!disk->total_lba)
            {
                continue;
            }
            int32 dev = device_install(
                    DEV_BLOCK, DEV_IDE_DISK, disk, disk->name, 0,
                    ide_pio_ioctl, ide_pio_read, ide_pio_write);
            for (size_t i = 0; i < IDE_PART_NR; i++)
            {
                struct ide_part_t *part = &disk->parts[i];
                if (!part->count)
                {
                    continue;
                }
                device_install(
                        DEV_BLOCK, DEV_IDE_PART, part, part->name, dev,
                        ide_pio_part_ioctl, ide_pio_part_read, ide_pio_part_write);
            }
        }
    }
}

void ide_init()
{
    LOGK("ide init...\n");
    ide_ctrl_init();
    ide_install();
}

void pio_test()
{
    uint16 *buf = (uint16 *)get_kernel_pages(1);
    struct ide_disk_t *disk = &controllers[0].disks[0];
    ide_pio_read(disk, buf, 4, 0);


    memset(buf, 0x5a, 512);

    ide_pio_write(disk, buf, 1, 1);

    free_page(KERNEL_POOL ,buf, 1);
    return;
}
