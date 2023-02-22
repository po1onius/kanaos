#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <types.h>
#include <list.h>
#include <lock.h>

#define BLOCK_SIZE 1024                       // 块大小
#define SECTOR_SIZE 512                       // 扇区大小
#define BLOCK_SECS (BLOCK_SIZE / SECTOR_SIZE) // 一块占 2 个扇区
 struct buffer_t
{
    int8 *data;        // 数据区
    int32 dev;         // 设备号
    uint32 block;       // 块号
    int32 count;         // 引用计数
    struct list_node hnode; // 哈希表拉链节点
    struct list_node rnode; // 缓冲节点
    struct lock lock;       // 锁
    bool dirty;        // 是否与磁盘不一致
    bool valid;        // 是否有效
};

struct buffer_t *getblk(int32 dev, uint32 block);
struct buffer_t *bread(int32 dev, uint32 block);
void bwrite(struct buffer_t *bf);
void brelse(struct buffer_t *bf);
void buffer_init();

#endif