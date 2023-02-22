#ifndef ___FS_H__
#define __FS_H__

#include <types.h>
#include <list.h>

#define BLOCK_SIZE 1024 // 块大小
#define SECTOR_SIZE 512 // 扇区大小

#define MINIX1_MAGIC 0x137F // 文件系统魔数
#define NAME_LEN 14        // 文件名长度

#define IMAP_NR 8
#define ZMAP_NR 8

#define BLOCK_BITS (BLOCK_SIZE * 8)                      // 块位图大小
#define BLOCK_INODES (BLOCK_SIZE / sizeof(struct inode_desc_t)) // 块 inode 数量
#define BLOCK_DENTRIES (BLOCK_SIZE / sizeof(struct dentry_t))   // 块 dentry 数量
#define BLOCK_INDEXES (BLOCK_SIZE / sizeof(uint16))         // 块索引数量

#define DIRECT_BLOCK (7)                                               // 直接块数量
#define INDIRECT1_BLOCK BLOCK_INDEXES                                  // 一级间接块数量
#define INDIRECT2_BLOCK (INDIRECT1_BLOCK * INDIRECT1_BLOCK)            // 二级间接块数量
#define TOTAL_BLOCK (DIRECT_BLOCK + INDIRECT1_BLOCK + INDIRECT2_BLOCK) // 全部块数量


enum file_flag
{
    O_RDONLY = 00,      // 只读方式
    O_WRONLY = 01,      // 只写方式
    O_RDWR = 02,        // 读写方式
    O_ACCMODE = 03,     // 文件访问模式屏蔽码
    O_CREAT = 00100,    // 如果文件不存在就创建
    O_EXCL = 00200,     // 独占使用文件标志
    O_NOCTTY = 00400,   // 不分配控制终端
    O_TRUNC = 01000,    // 若文件已存在且是写操作，则长度截为 0
    O_APPEND = 02000,   // 以添加方式打开，文件指针置为文件尾
    O_NONBLOCK = 04000, // 非阻塞方式打开和操作文件
};

struct inode_desc_t
{
    uint16 mode;    // 文件类型和属性(rwx 位)
    uint16 uid;     // 用户id（文件拥有者标识符）
    uint32 size;    // 文件大小（字节数）
    uint32 mtime;   // 修改时间戳 这个时间戳应该用 UTC 时间，不然有瑕疵
    uint8 gid;      // 组id(文件拥有者所在的组)
    uint8 nlinks;   // 链接数（多少个文件目录项指向该i 节点）
    uint16 zone[9]; // 直接 (0-6)、间接(7)或双重间接 (8) 逻辑块号
};

struct inode_t
{
    struct inode_desc_t *desc;   // inode 描述符
    struct buffer_t *buf; // inode 描述符对应 buffer
    int32 dev;            // 设备号
    uint32 nr;             // i 节点号
    uint32 count;            // 引用计数
    time_t atime;         // 访问时间
    time_t ctime;         // 创建时间
    struct list_node node;     // 链表结点
    int32 mount;          // 安装设备
};

struct file_t
{
    struct inode_t *inode; // 文件 inode
    uint32 count;      // 引用计数
    int32 offset;   // 文件偏移
    int32 flags;      // 文件标记
    int32 mode;       // 文件模式
};

enum whence_t
{
    SEEK_SET = 1, // 直接设置偏移
    SEEK_CUR,     // 当前位置偏移
    SEEK_END      // 结束位置偏移
};

struct super_desc_t
{
    uint16 inodes;        // 节点数
    uint16 zones;         // 逻辑块数
    uint16 imap_blocks;   // i 节点位图所占用的数据块数
    uint16 zmap_blocks;   // 逻辑块位图所占用的数据块数
    uint16 firstdatazone; // 第一个数据逻辑块号
    uint16 log_zone_size; // log2(每逻辑块数据块数)
    uint32 max_size;      // 文件最大长度
    uint16 magic;         // 文件系统魔数
};


struct super_block_t
{
    struct super_desc_t *desc;              // 超级块描述符
    struct buffer_t *buf;            // 超级快描述符 buffer
    struct buffer_t *imaps[IMAP_NR]; // inode 位图缓冲
    struct buffer_t *zmaps[ZMAP_NR]; // 块位图缓冲
    int32 dev;                       // 设备号
    struct list inode_list;               // 使用中 inode 链表
    struct inode_t *iroot;                  // 根目录 inode
    struct inode_t *imount;                 // 安装到的 inode
};

// 文件目录项结构
struct dentry_t
{
    uint16 nr;              // i 节点
    int8 name[NAME_LEN]; // 文件名
};

void super_init();
struct super_block_t *get_super(int32 dev);  // 获得 dev 对应的超级块
struct super_block_t *read_super(int32 dev); // 读取 dev 对应的超级块

uint32 balloc(int32 dev);          // 分配一个文件块
void bfree(int32 dev, uint32 idx); // 释放一个文件块
uint32 ialloc(int32 dev);          // 分配一个文件系统 inode
void ifree(int32 dev, uint32 idx); // 释放一个文件系统 inode

// 获取 inode 第 block 块的索引值
// 如果不存在 且 create 为 true，则创建
uint32 bmap(struct inode_t *inode, uint32 block, bool create);

void inode_init();
struct inode_t *get_root_inode();          // 获取根目录 inode
struct inode_t *iget(int32 dev, uint32 nr); // 获得设备 dev 的 nr inode
void iput(struct inode_t *inode);          // 释放 inode

struct inode_t *inode_open(int8 *pathname, int32 flag, int32 mode);

struct inode_t *named(int8 *pathname, int8 **next); // 获取 pathname 对应的父目录 inode
struct inode_t *namei(int8 *pathname);              // 获取 pathname 对应的 inode

// 从 inode 的 offset 处，读 len 个字节到 buf
int32 inode_read(struct inode_t *inode, int8 *buf, uint32 len, int32 offset);

// 从 inode 的 offset 处，将 buf 的 len 个字节写入磁盘
int32 inode_write(struct inode_t *inode, int8 *buf, uint32 len, int32 offset);

// 释放 inode 所有文件块
void inode_truncate(struct inode_t *inode);

void file_init();
struct file_t *get_file();
void put_file(struct file_t *file);

#endif
