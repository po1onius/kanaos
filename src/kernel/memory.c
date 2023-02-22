#include <memory.h>
#include <types.h>
#include <kanaos.h>
#include <string.h>
#include <printk.h>
#include <debug.h>
#include <gdt.h>
#include <assert.h>
#include <bitmap.h>
#include <thread.h>
#include <process.h>
#include <intr.h>

static uint32 phy_valid_start_addr = 0;
static uint32 phy_valid_len = 0;


struct mem_pool k_pool, u_pool;
struct vaddr_pool k_vaddr_pool;

struct mem_block_desc k_block_desc[DESC_CNT];

void block_desc_init(struct mem_block_desc* block_desc)
{
    uint32 block_size = 16;
    for(size_t i = 0; i < DESC_CNT; i++)
    {
        block_desc[i].block_size = block_size;
        block_desc[i].blocks_per_arena = (PAGE_SIZE - sizeof(struct arena)) / block_size;
        list_init(&block_desc[i].free_list);
        block_size *= 2;
    }
}

struct mem_block* get_arena_block(struct arena* arena, uint32 index)
{
    return (struct mem_block*)((uint32)arena + sizeof(struct arena) + index * arena->desc->block_size);
}

struct arena* block_in_arena(struct mem_block* block)
{
    return (struct arena*)((uint32)block & 0xfffff000);
}



void mem_pool_init()
{
    uint32 used_mem = MEMORY_BASE + PAGE_SIZE * 256;//pdt(1)+pet(1)+pet(254)
    uint32 free_mem_start = used_mem;
    uint32 free_mem_len = phy_valid_len + phy_valid_start_addr - used_mem;

    uint32 free_pages_count = free_mem_len / PAGE_SIZE;

    uint32 kernel_pages_count = free_pages_count / 2;
    uint32 user_pages_count = free_pages_count - kernel_pages_count;

    uint32 kernel_phy_start = free_mem_start;
    uint32 user_phy_start = kernel_phy_start + kernel_pages_count * PAGE_SIZE;


    uint32 kernel_bitmap_len = kernel_pages_count / 8;
    uint32 user_bitmap_len = user_pages_count / 8;


    k_pool.pool_bitmap.base = (int8*)BITMAP_BASE;
    k_pool.pool_bitmap.len = kernel_bitmap_len;
    k_pool.phy_start = kernel_phy_start;
    k_pool.len = kernel_pages_count * PAGE_SIZE;
    lock_init(&k_pool.lock);

    u_pool.pool_bitmap.base = (int8*)(BITMAP_BASE + kernel_bitmap_len);
    u_pool.pool_bitmap.len = user_bitmap_len;
    u_pool.phy_start = user_phy_start;
    u_pool.len = user_pages_count * PAGE_SIZE;
    lock_init(&u_pool.lock);

    bitmap_init(&k_pool.pool_bitmap);
    bitmap_init(&u_pool.pool_bitmap);

    k_vaddr_pool.vaddr_bitmap.base = (int8*)(BITMAP_BASE + kernel_bitmap_len + user_bitmap_len);
    k_vaddr_pool.vaddr_bitmap.len = kernel_bitmap_len;
    k_vaddr_pool.vaddr_start = K_HEAP_START;
    bitmap_init(&k_vaddr_pool.vaddr_bitmap);
}

void* get_page_vaddr(enum pool_type type, uint32 count)
{
    uint32 bit_index_start = -1;
    uint32 vaddr = 0;
    if(type == KERNEL_POOL)
    {
        bit_index_start = bitmap_scan(&k_vaddr_pool.vaddr_bitmap, count);
        if (bit_index_start == -1)
        {
            return NULL;
        }
        for (int32 i = 0; i < count; i++)
        {
            bitmap_set(&k_vaddr_pool.vaddr_bitmap, bit_index_start + i, 1);
        }
        vaddr = k_vaddr_pool.vaddr_start + bit_index_start * PAGE_SIZE;
    }
    else
    {
        struct task_struct *cur = running_thread();
        bit_index_start = bitmap_scan(&cur->user_vaddr.vaddr_bitmap, count);
        if (bit_index_start == -1)
        {
            return NULL;
        }
        for (int32 i = 0; i < count; i++)
        {
            bitmap_set(&cur->user_vaddr.vaddr_bitmap, bit_index_start + i, 1);
        }
        vaddr = cur->user_vaddr.vaddr_start + bit_index_start * PAGE_SIZE;

        /* (0xc0000000 - PG_SIZE)做为用户3级栈已经在start_process被分配 */
        assert((uint32) vaddr < (0xc0000000 - PAGE_SIZE));
    }
    return (void*)vaddr;
}

void* get_page_phy(struct mem_pool* pool)
{
    uint32 bit_index_start = bitmap_scan(&pool->pool_bitmap, 1);
    if(bit_index_start == -1)
    {
        return NULL;
    }
    bitmap_set(&pool->pool_bitmap, bit_index_start, 1);
    return (void*)(pool->phy_start + bit_index_start * PAGE_SIZE);
}

uint32* vaddr_in_pdt(uint32 vaddr)
{
    return (uint32*)(0xfffff000 + ((vaddr & 0xffc00000) >> 22) * 4);
}

uint32* vaddr_in_pt(uint32 vaddr)
{
    return (uint32*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + ((vaddr & 0x3ff000) >> 12) * 4);
}

void phy_vaddr_page_bind(void* phy, void* vaddr)
{

    uint32* pde = vaddr_in_pdt((uint32)vaddr);
    uint32* pte = vaddr_in_pt((uint32)vaddr);
    if(*pde & 0x01)
    {
        assert(!(*pte & 0x01));
        if(!(*pte & 0x01))
        {
            *pte = ((uint32)(phy) | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else
    {
        uint32 pde_phyaddr = (uint32)get_page_phy(&k_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        memset((void*)((uint32)pte & 0xfffff000), 0, PAGE_SIZE);
        assert(!(*pte & 0x01));
        *pte = ((uint32)phy | PG_US_U | PG_RW_W | PG_P_1);
    }
}


void* malloc_page(enum pool_type type, uint32 count)
{
    void* vaddr = get_page_vaddr(type, count);
    uint32 addr = (uint32)vaddr;
    if(vaddr == NULL)
    {
        return NULL;
    }
    struct mem_pool* pool = (type == KERNEL_POOL) ? &k_pool : &u_pool;
    while(count-- > 0)
    {
        void* phy = get_page_phy(pool);
        if(phy == NULL)
        {
            return NULL;
        }
        phy_vaddr_page_bind(phy, (void*)addr);
        addr += PAGE_SIZE;
    }
    return vaddr;
}

void* get_kernel_pages(uint32 count)
{
    lock_acquire(&k_pool.lock);
    void* vaddr =  malloc_page(KERNEL_POOL, count);
    if (vaddr != NULL)// 若分配的地址不为空,将页框清0后返回
    {
        memset(vaddr, 0, count * PAGE_SIZE);
    }
    lock_release(&k_pool.lock);
    return vaddr;
}

void* get_user_pages(uint32 count)
{
    lock_acquire(&u_pool.lock);
    void* vaddr = malloc_page(USER_POOL, count);
    memset(vaddr, 0, count * PAGE_SIZE);
    lock_release(&k_pool.lock);
    return vaddr;
}



void set_page_entry(struct page_entry_t* entry, uint32 addr)
{
    memset(entry, 0, sizeof(struct page_entry_t));
    entry->present = 1;
    entry->write = 1;
    entry->user = 1;
    entry->addr = addr >> 12;
}

void setup_page_table()
{

    uint32 pet_base = PDT_BASE + 0x1000;
    memset((void*)PDT_BASE, 0, PAGE_SIZE);
    memset((void*)pet_base, 0, PAGE_SIZE);


    set_page_entry((struct page_entry_t*)PDT_BASE, pet_base);
    for(int32 i = 768; i < 1023; i++)
    {
        set_page_entry((struct page_entry_t*)(PDT_BASE) + i, pet_base + (i - 768) * 0x1000);
    }
    set_page_entry((struct page_entry_t*)(PDT_BASE) + 1023, PDT_BASE);

    struct page_entry_t* pet = (struct page_entry_t*)pet_base;
    for(int32 i = 0; i < 256; i++)
    {
        set_page_entry(pet + i, i * 0x1000);
    }

    /*
    struct page_entry_t *pde = (struct page_entry_t *)PDT_BASE;
    memset(pde, 0, PAGE_SIZE);

    set_page_entry(&pde[0], PDT_BASE + 0x1000);

    struct page_entry_t *pte = (struct page_entry_t *)(PDT_BASE + 0x1000);
    memset(pte, 0, PAGE_SIZE);

    struct page_entry_t *entry;
    for (size_t tidx = 0; tidx < 1024; tidx++)
    {
        entry = &pte[tidx];
        set_page_entry(entry, tidx << 12);
        //memory_map[tidx] = 1; // 设置物理内存数组，该页被占用
    }
     */
}

void* get_a_page(enum pool_type pf, uint32 vaddr)
{
    struct mem_pool* mem_pool = pf & KERNEL_POOL ? &k_pool : &u_pool;
    lock_acquire(&mem_pool->lock);

    /* 先将虚拟地址对应的位图置1 */
    struct task_struct* cur = running_thread();
    int32 bit_idx = -1;

/* 若当前是用户进程申请用户内存,就修改用户进程自己的虚拟地址位图 */
    if (cur->pgdir != NULL && pf == USER_POOL)
    {
        bit_idx = (vaddr - cur->user_vaddr.vaddr_start) / PAGE_SIZE;
        assert(bit_idx > 0);
        bitmap_set(&cur->user_vaddr.vaddr_bitmap, bit_idx, 1);

    }
    else if (cur->pgdir == NULL && pf == KERNEL_POOL)
    {
/* 如果是内核线程申请内核内存,就修改kernel_vaddr. */
        bit_idx = (vaddr - k_vaddr_pool.vaddr_start) / PAGE_SIZE;
        assert(bit_idx > 0);
        bitmap_set(&k_vaddr_pool.vaddr_bitmap, bit_idx, 1);
    }
    else
    {
        //PANIC("get_a_page:not allow kernel alloc userspace or user alloc kernelspace by get_a_page");
    }
    void* page_phyaddr = get_page_phy(mem_pool);
    if (page_phyaddr == NULL)
    {
        return NULL;
    }
    phy_vaddr_page_bind(page_phyaddr, (void*)vaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}



uint32 addr_v2p(uint32 vaddr)
{
    uint32* pte = vaddr_in_pt(vaddr);
/* (*pte)的值是页表所在的物理页框地址,
 * 去掉其低12位的页表项属性+虚拟地址vaddr的低12位 */
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

// 得到 cr3 寄存器
uint32 get_cr3()
{
    // 直接将 mov eax, cr3，返回值在 eax 中
    asm volatile("movl %cr3, %eax\n");
}

// 设置 cr3 寄存器，参数是页目录的地址
void set_cr3(uint32 pde)
{
    asm volatile("movl %%eax, %%cr3\n" ::"a"(pde));
}

void set_page_mode_gdtr()
{
    asm volatile("sgdt gdtr");
    gdtr.base += 0xc0000000;
    asm volatile("lgdt gdtr\n");
}

// 将 cr0 寄存器最高位 PE 置为 1，启用分页
static void enable_page()
{
    // 0b1000_0000_0000_0000_0000_0000_0000_0000
    // 0x80000000
    asm volatile(
    "add $0xc0000000, %esp\n"
    "movl %cr0, %eax\n"
    "orl $0x80000000, %eax\n"
    "movl %eax, %cr0\n");
}

/* 安装1页大小的vaddr,专门针对fork时虚拟地址位图无须操作的情况 */
void* get_a_page_without_opvaddrbitmap(enum pool_type pf, uint32 vaddr)
{
    struct mem_pool* mem_pool = pf & KERNEL_POOL ? &k_pool : &u_pool;
    enum intr_status old_status = intr_disable();
    void* page_phyaddr = get_page_phy(mem_pool);
    if (page_phyaddr == NULL)
    {
        lock_release(&mem_pool->lock);
        return NULL;
    }
    phy_vaddr_page_bind(page_phyaddr, (void*)vaddr);
    intr_set_status(old_status);
    return (void*)vaddr;
}

/* 在堆中申请size字节内存 */
void* sys_malloc(uint32 size)
{
    enum pool_type PF;
    struct mem_pool* mem_pool;
    uint32 pool_size;
    struct mem_block_desc* descs;
    struct task_struct* cur_thread = running_thread();

/* 判断用哪个内存池*/
    if (cur_thread->pgdir == NULL)
    {     // 若为内核线程
        PF = KERNEL_POOL;
        pool_size = k_pool.len;
        mem_pool = &k_pool;
        descs = k_block_desc;
    }
    else
    {				      // 用户进程pcb中的pgdir会在为其分配页表时创建
        PF = USER_POOL;
        pool_size = u_pool.len;
        mem_pool = &u_pool;
        descs = cur_thread->u_block_desc;
    }

    /* 若申请的内存不在内存池容量范围内则直接返回NULL */
    if (!(size > 0 && size < pool_size))
    {
        return NULL;
    }
    struct arena* a;
    struct mem_block* b;
    lock_acquire(&mem_pool->lock);

/* 超过最大内存块1024, 就分配页框 */
    if (size > 1024)
    {
        uint32 page_cnt = DIV_ROUND_UP(size + sizeof(struct arena), PAGE_SIZE);    // 向上取整需要的页框数

        a = malloc_page(PF, page_cnt);

        if (a != NULL)
        {
            memset(a, 0, page_cnt * PAGE_SIZE);	 // 将分配的内存清0

            /* 对于分配的大块页框,将desc置为NULL, cnt置为页框数,large置为true */
            a->desc = NULL;
            a->cnt = page_cnt;
            a->large = true;
            lock_release(&mem_pool->lock);
            return (void*)(a + 1);		 // 跨过arena大小，把剩下的内存返回
        }
        else
        {
            lock_release(&mem_pool->lock);
            return NULL;
        }
    }
    else
    {    // 若申请的内存小于等于1024,可在各种规格的mem_block_desc中去适配
        uint8 desc_idx;

        /* 从内存块描述符中匹配合适的内存块规格 */
        for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++)
        {
            if (size <= descs[desc_idx].block_size)
            {  // 从小往大后,找到后退出
                break;
            }
        }

        /* 若mem_block_desc的free_list中已经没有可用的mem_block,
         * 就创建新的arena提供mem_block */
        if (list_empty(&descs[desc_idx].free_list))
        {
            a = malloc_page(PF, 1);       // 分配1页框做为arena
            if (a == NULL)
            {
                lock_release(&mem_pool->lock);
                return NULL;
            }
            memset(a, 0, PAGE_SIZE);

            /* 对于分配的小块内存,将desc置为相应内存块描述符,
             * cnt置为此arena可用的内存块数,large置为false */
            a->desc = &descs[desc_idx];
            a->large = false;
            a->cnt = descs[desc_idx].blocks_per_arena;
            uint32 block_idx;

            enum intr_status old_status = intr_disable();

            /* 开始将arena拆分成内存块,并添加到内存块描述符的free_list中 */
            for (block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++)
            {
                b = get_arena_block(a, block_idx);
                assert(!elem_find(&a->desc->free_list, &b->free_elem));
                list_append(&a->desc->free_list, &b->free_elem);
            }
            intr_set_status(old_status);
        }

        /* 开始分配内存块 */
        b = elem2entry(struct mem_block, free_elem, list_pop(&(descs[desc_idx].free_list)));
        memset(b, 0, descs[desc_idx].block_size);

        a = block_in_arena(b);  // 获取内存块b所在的arena
        a->cnt--;		   // 将此arena中的空闲内存块数减1
        lock_release(&mem_pool->lock);
        return (void*)b;
    }
}

/* 将物理地址pg_phy_addr回收到物理内存池 */
void free_page_phy(uint32 pg_phy_addr)
{
    struct mem_pool* mem_pool;
    uint32 bit_idx = 0;
    if (pg_phy_addr >= u_pool.phy_start)
    {     // 用户物理内存池
        mem_pool = &u_pool;
        bit_idx = (pg_phy_addr - u_pool.phy_start) / PAGE_SIZE;
    }
    else
    {	  // 内核物理内存池
        mem_pool = &k_pool;
        bit_idx = (pg_phy_addr - k_pool.phy_start) / PAGE_SIZE;
    }
    bitmap_set(&mem_pool->pool_bitmap, bit_idx, 0);	 // 将位图中该位清0
}

/* 去掉页表中虚拟地址vaddr的映射,只去掉vaddr对应的pte */
static void page_table_pte_remove(uint32 vaddr)
{
    uint32* pte = vaddr_in_pt(vaddr);
    *pte &= ~PG_P_1;	// 将页表项pte的P位置0
    asm volatile ("invlpg %0"::"m" (vaddr):"memory");    //更新tlb
}

/* 在虚拟地址池中释放以_vaddr起始的连续pg_cnt个虚拟页地址 */
static void vaddr_remove(enum pool_type pf, void* _vaddr, uint32 pg_cnt)
{
    uint32 bit_idx_start = 0;
    uint32 vaddr = (uint32)_vaddr;
    uint32 cnt = 0;

    if (pf == KERNEL_POOL)
    {  // 内核虚拟内存池
        bit_idx_start = (vaddr - k_vaddr_pool.vaddr_start) / PAGE_SIZE;
        while(cnt < pg_cnt)
        {
            bitmap_set(&k_vaddr_pool.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
    else
    {  // 用户虚拟内存池
        struct task_struct* cur_thread = running_thread();
        bit_idx_start = (vaddr - cur_thread->user_vaddr.vaddr_start) / PAGE_SIZE;
        while(cnt < pg_cnt)
        {
            bitmap_set(&cur_thread->user_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 0);
        }
    }
}

/* 释放以虚拟地址vaddr为起始的cnt个物理页框 */
void free_page(enum pool_type pf, void* _vaddr, uint32 pg_cnt)
{
    uint32 pg_phy_addr;
    uint32 vaddr = (uint32)_vaddr;
    uint32 page_cnt = 0;
    assert(pg_cnt >=1 && vaddr % PAGE_SIZE == 0);
    pg_phy_addr = addr_v2p(vaddr);  // 获取虚拟地址vaddr对应的物理地址

/* 确保待释放的物理内存在低端1M+1k大小的页目录+1k大小的页表地址范围外 */
    assert((pg_phy_addr % PAGE_SIZE) == 0 && pg_phy_addr >= 0x102000);

/* 判断pg_phy_addr属于用户物理内存池还是内核物理内存池 */
    if (pg_phy_addr >= u_pool.phy_start)
    {   // 位于user_pool内存池
        vaddr -= PAGE_SIZE;
        while (page_cnt < pg_cnt)
        {
            vaddr += PAGE_SIZE;
            pg_phy_addr = addr_v2p(vaddr);

            /* 确保物理地址属于用户物理内存池 */
            assert((pg_phy_addr % PAGE_SIZE) == 0 && pg_phy_addr >= u_pool.phy_start);

            /* 先将对应的物理页框归还到内存池 */
            free_page_phy(pg_phy_addr);

            /* 再从页表中清除此虚拟地址所在的页表项pte */
            page_table_pte_remove(vaddr);

            page_cnt++;
        }
        /* 清空虚拟地址的位图中的相应位 */
        vaddr_remove(pf, _vaddr, pg_cnt);

    }
    else
    {	     // 位于kernel_pool内存池
        vaddr -= PAGE_SIZE;
        while (page_cnt < pg_cnt)
        {
            vaddr += PAGE_SIZE;
            pg_phy_addr = addr_v2p(vaddr);
            /* 确保待释放的物理内存只属于内核物理内存池 */
            assert((pg_phy_addr % PAGE_SIZE) == 0 && pg_phy_addr >= k_pool.phy_start && pg_phy_addr < u_pool.phy_start);
            /* 先将对应的物理页框归还到内存池 */
            free_page_phy(pg_phy_addr);

            /* 再从页表中清除此虚拟地址所在的页表项pte */
            page_table_pte_remove(vaddr);

            page_cnt++;
        }
        /* 清空虚拟地址的位图中的相应位 */
        vaddr_remove(pf, _vaddr, pg_cnt);
    }
}

/* 回收内存ptr */
void sys_free(void* addr)
{
    assert(addr != NULL);
    if (addr != NULL)
    {
        enum pool_type PF;
        struct mem_pool* mem_pool;

        /* 判断是线程还是进程 */
        if (running_thread()->pgdir == NULL)
        {
            assert((uint32)addr >= K_HEAP_START);
            PF = KERNEL_POOL;
            mem_pool = &k_pool;
        }
        else
        {
            PF = USER_POOL;
            mem_pool = &u_pool;
        }

        lock_acquire(&mem_pool->lock);
        struct mem_block* b = addr;
        struct arena* a = block_in_arena(b);	     // 把mem_block转换成arena,获取元信息
        assert(a->large == 0 || a->large == 1);
        if (a->desc == NULL && a->large == true)
        { // 大于1024的内存
            free_page(PF, a, a->cnt);
        }
        else
        {				 // 小于等于1024的内存块
            /* 先将内存块回收到free_list */
            list_append(&a->desc->free_list, &b->free_elem);

            /* 再判断此arena中的内存块是否都是空闲,如果是就释放arena */
            if (++a->cnt == a->desc->blocks_per_arena)
            {
                uint32 block_idx;
                for (block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++)
                {
                    struct mem_block*  t = get_arena_block(a, block_idx);
                    assert(elem_find(&a->desc->free_list, &t->free_elem));
                    list_remove(&t->free_elem);
                }
                free_page(PF, a, 1);
            }
        }
        lock_release(&mem_pool->lock);
    }
}

void memory_init(uint32 magic, uint32 addr)
{
    if(magic == KANAOS_MAGIC)
    {
        int32 ards_n = *(int32*)addr;
        struct ards_t* ards_buffer = (struct ards_t*)((int16*)(addr) + 1);
        for(size_t i = 0; i < ards_n; i++)
        {
            struct ards_t t = *(ards_buffer + i);
            if(t.type == 1 && (uint32)t.size > phy_valid_len)
            {
                phy_valid_start_addr = (uint32)t.base;
                phy_valid_len = (uint32)t.size;
            }
        }
    }
    assert(phy_valid_start_addr == MEMORY_BASE);
    mem_pool_init();
    block_desc_init(k_block_desc);
}
