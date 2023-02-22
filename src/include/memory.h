#ifndef __MEMORY_H__
#define __MEMORY_H__

#define PAGE_SIZE 0x1000
#define MEMORY_BASE 0x100000

#define BITMAP_BASE 0xc0090000
#define K_HEAP_START 0xc0100000

#include <types.h>
#include <bitmap.h>
#include <lock.h>

struct ards_t
{
    uint64 base;
    uint64 size;
    uint32 type;
}_packed;

struct page_entry_t
{
    uint8 present : 1;  // 在内存中
    uint8 write : 1;    // 0 只读 1 可读可写
    uint8 user : 1;     // 1 所有人 0 超级用户 DPL < 3
    uint8 pwt : 1;      // page write through 1 直写模式，0 回写模式
    uint8 pcd : 1;      // page cache disable 禁止该页缓冲
    uint8 accessed : 1; // 被访问过，用于统计使用频率
    uint8 dirty : 1;    // 脏页，表示该页缓冲被写过
    uint8 pat : 1;      // page attribute table 页大小 4K/4M
    uint8 global : 1;   // 全局，所有进程都用到了，该页不刷新缓冲
    uint8 ignored : 3;  // 该安排的都安排了，送给操作系统吧
    uint32 addr : 20;  //页地址
}_packed;

struct mem_pool
{
    struct bitmap_t pool_bitmap;
    uint32 phy_start;
    uint32 len;
    struct lock lock;
};

struct vaddr_pool
{
    struct bitmap_t vaddr_bitmap;
    uint32 vaddr_start;
};

enum pool_type
{
    KERNEL_POOL = 1,
    USER_POOL
};

/* 内存块 */
struct mem_block 
{
   struct list_node free_elem;
};

/* 内存块描述符 */
struct mem_block_desc 
{
   uint32 block_size;		 // 内存块大小
   uint32 blocks_per_arena;	 // 本arena中可容纳此mem_block的数量.
   struct list free_list;	 // 目前可用的mem_block链表
};

#define DESC_CNT 7	   // 内存块描述符个数

/* 内存仓库arena元信息 */
struct arena
{
    struct mem_block_desc* desc;	 // 此arena关联的mem_block_desc
/* large为ture时,cnt表示的是页框数。
 * 否则cnt表示空闲mem_block数量 */
    uint32 cnt;
    bool large;
};


#define PDT_BASE 0x100000

#define	 PG_P_1	  1	// 页表项或页目录项存在属性位
#define	 PG_P_0	  0	// 页表项或页目录项存在属性位
#define	 PG_RW_R  0	// R/W 属性位值, 读/执行
#define	 PG_RW_W  2	// R/W 属性位值, 读/写/执行
#define	 PG_US_S  0	// U/S 属性位值, 系统级
#define	 PG_US_U  4	// U/S 属性位值, 用户级


void block_desc_init(struct mem_block_desc* block_desc);
void* get_kernel_pages(uint32 count);
void* get_user_pages(uint32 count);
uint32 addr_v2p(uint32 vaddr);
void* get_a_page(enum pool_type pf, uint32 vaddr);
void set_cr3(uint32 pde);
void* sys_malloc(uint32 size);
void sys_free(void* addr);
void free_page(enum pool_type pf, void* _vaddr, uint32 pg_cnt);
void* get_a_page_without_opvaddrbitmap(enum pool_type pf, uint32 vaddr);

#endif