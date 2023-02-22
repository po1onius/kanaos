#include <fs.h>
#include <stat.h>
#include <syscall.h>
#include <assert.h>
#include <debug.h>
#include <buffer.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <clock.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define INODE_NR 64

static struct inode_t inode_table[INODE_NR];

// 申请一个 inode
static struct inode_t *get_free_inode()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        struct inode_t *inode = &inode_table[i];
        if (inode->dev == EOF)
        {
            return inode;
        }
    }
    panic("no more inode!!!");
}

// 释放一个 inode
static void put_free_inode(struct inode_t *inode)
{
    assert(inode != inode_table);
    assert(inode->count == 0);
    inode->dev = EOF;
}

// 获取根 inode
struct inode_t *get_root_inode()
{
    return inode_table;
}

// 计算 inode nr 对应的块号
static inline uint32 inode_block(struct super_block_t *sb, uint32 nr)
{
    // inode 编号 从 1 开始
    return 2 + sb->desc->imap_blocks + sb->desc->zmap_blocks + (nr - 1) / BLOCK_INODES;
}

// 从已有 inode 中查找编号为 nr 的 inode
static struct inode_t *find_inode(int32 dev, uint32 nr)
{
    struct super_block_t *sb = get_super(dev);
    assert(sb);
    struct list *list = &sb->inode_list;

    for (struct list_node *node = list->head.next; node != &list->tail; node = node->next)
    {
        struct inode_t *inode = elem2entry(struct inode_t, node, node);
        if (inode->nr == nr)
        {
            return inode;
        }
    }
    return NULL;
}

// 获得设备 dev 的 nr inode
struct inode_t *iget(int32 dev, uint32 nr)
{
    struct inode_t *inode = find_inode(dev, nr);
    if (inode)
    {
        inode->count++;
        inode->atime = time();

        return inode;
    }

    struct super_block_t *sb = get_super(dev);
    assert(sb);

    assert(nr <= sb->desc->inodes);

    inode = get_free_inode();
    inode->dev = dev;
    inode->nr = nr;
    inode->count++;

    // 加入超级块 inode 链表
    list_push(&sb->inode_list, &inode->node);

    uint32 block = inode_block(sb, inode->nr);
    struct buffer_t *buf = bread(inode->dev, block);

    inode->buf = buf;

    // 将缓冲视为一个 inode 描述符数组，获取对应的指针；
    inode->desc = &((struct inode_desc_t *)buf->data)[(inode->nr - 1) % BLOCK_INODES];

    inode->ctime = inode->desc->mtime;
    inode->atime = time();

    return inode;
}

// 释放 inode
void iput(struct inode_t *inode)
{
    if (!inode)
    {
        return;
    }

    // TODO: need write... ?
    if (inode->buf->dirty)
    {
        bwrite(inode->buf);
    }

    inode->count--;

    if (inode->count)
    {
        return;
    }

    // 释放 inode 对应的缓冲
    brelse(inode->buf);

    // 从超级块链表中移除
    list_remove(&inode->node);

    // 释放 inode 内存
    put_free_inode(inode);
}

void inode_init()
{
    for (size_t i = 0; i < INODE_NR; i++)
    {
        struct inode_t *inode = &inode_table[i];
        inode->dev = EOF;
    }
}

// 从 inode 的 offset 处，读 len 个字节到 buf
int32 inode_read(struct inode_t *inode, int8 *buf, uint32 len, int32 offset)
{
    assert(ISFILE(inode->desc->mode) || ISDIR(inode->desc->mode));

    // 如果偏移量超过文件大小，返回 EOF
    if (offset >= inode->desc->size)
    {
        return EOF;
    }

    // 开始读取的位置
    uint32 begin = offset;

    // 剩余字节数
    uint32 left = MIN(len, inode->desc->size - offset);
    while (left)
    {
        // 找到对应的文件便宜，所在文件块
        uint32 nr = bmap(inode, offset / BLOCK_SIZE, false);
        assert(nr);

        // 读取文件块缓冲
        struct buffer_t *bf = bread(inode->dev, nr);

        // 文件块中的偏移量
        uint32 start = offset % BLOCK_SIZE;

        // 本次需要读取的字节数
        uint32 chars = MIN(BLOCK_SIZE - start, left);

        // 更新 偏移量 和 剩余字节数
        offset += chars;
        left -= chars;

        // 文件块中的指针
        int8 *ptr = bf->data + start;

        // 拷贝内容
        memcpy(buf, ptr, chars);

        // 更新缓存位置
        buf += chars;

        // 释放文件块缓冲
        brelse(bf);
    }

    // 更新访问时间
    inode->atime = time();

    // 返回读取数量
    return offset - begin;
}

// 从 inode 的 offset 处，将 buf 的 len 个字节写入磁盘
int32 inode_write(struct inode_t *inode, int8 *buf, uint32 len, int32 offset)
{
    // 不允许目录写入目录文件，修改目录有其他的专用方法
    assert(ISFILE(inode->desc->mode));

    // 开始的位置
    uint32 begin = offset;

    // 剩余数量
    uint32 left = len;

    while (left)
    {
        // 找到文件块，若不存在则创建
        uint32 nr = bmap(inode, offset / BLOCK_SIZE, true);
        assert(nr);

        // 将读入文件块
        struct buffer_t *bf = bread(inode->dev, nr);
        bf->dirty = true;

        // 块中的偏移量
        uint32 start = offset % BLOCK_SIZE;
        // 文件块中的指针
        int8 *ptr = bf->data + start;

        // 读取的数量
        uint32 chars = MIN(BLOCK_SIZE - start, left);

        // 更新偏移量
        offset += chars;

        // 更新剩余字节数
        left -= chars;

        // 如果偏移量大于文件大小，则更新
        if (offset > inode->desc->size)
        {
            inode->desc->size = offset;
            inode->buf->dirty = true;
        }

        // 拷贝内容
        memcpy(ptr, buf, chars);

        // 更新缓存偏移
        buf += chars;

        // 释放文件块
        brelse(bf);
    }

    // 更新修改时间
    inode->desc->mtime = inode->atime = time();

    // TODO: 写入磁盘 ？
    bwrite(inode->buf);

    // 返回写入大小
    return offset - begin;
}

static void inode_bfree(struct inode_t *inode, uint16 *array, int32 index, int32 level)
{
    if (!array[index])
    {
        return;
    }

    if (!level)
    {
        bfree(inode->dev, array[index]);
        return;
    }

    struct buffer_t *buf = bread(inode->dev, array[index]);
    for (size_t i = 0; i < BLOCK_INDEXES; i++)
    {
        inode_bfree(inode, (uint16 *)buf->data, i, level - 1);
    }
    brelse(buf);
    bfree(inode->dev, array[index]);
}

// 释放 inode 所有文件块
void inode_truncate(struct inode_t *inode)
{
    if (!ISFILE(inode->desc->mode) && !ISDIR(inode->desc->mode))
    {
        return;
    }

    // 释放直接块
    for (size_t i = 0; i < DIRECT_BLOCK; i++)
    {
        inode_bfree(inode, inode->desc->zone, i, 0);
        inode->desc->zone[i] = 0;
    }

    // 释放一级间接块
    inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK, 1);
    inode->desc->zone[DIRECT_BLOCK] = 0;

    // 释放二级间接块
    inode_bfree(inode, inode->desc->zone, DIRECT_BLOCK + 1, 2);
    inode->desc->zone[DIRECT_BLOCK + 1] = 0;

    inode->desc->size = 0;
    inode->buf->dirty = true;
    inode->desc->mtime = time();
    bwrite(inode->buf);
}
