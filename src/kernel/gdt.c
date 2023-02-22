#include <gdt.h>
#include <string.h>



struct descriptor gdt[GDT_SIZE];
struct dt_ptr gdtr;

//之前gdt在loader里,这里重新加载在内核里

void make_gdt_desc(struct descriptor* desc, uint32 base, uint32 limit, uint8 dpl, uint8 type)
{
    desc->base_low = base & 0xffffff;
    desc->base_high = (base >> 24) & 0xff;
    desc->limit_low = limit & 0xffff;
    desc->limit_high = (limit >> 16) & 0xf;
    desc->segment = 1;
    desc->granularity = 1;
    desc->big = 1;
    desc->long_mode = 0;
    desc->present = 1;
    desc->DPL = dpl;
    desc->type = type;
}

void gdt_init()
{
    struct descriptor* k_code = gdt + KERNEL_CODE_IDX;
    make_gdt_desc(k_code, 0, 0xfffff, 0, 0b1010);

    struct descriptor* k_data = gdt + KERNEL_DATA_IDX;
    make_gdt_desc(k_data, 0, 0xfffff, 0, 0b0010);

    struct descriptor* u_code = gdt + USER_CODE_IDX;
    make_gdt_desc(u_code, 0, 0xfffff, 3, 0b1010);

    struct descriptor* u_data = gdt + USER_DATA_IDX;
    make_gdt_desc(u_data, 0, 0xfffff, 3, 0b0010);

    gdtr.base = (uint32)gdt;
    gdtr.limit = sizeof(gdt) - 1;
    asm volatile("lgdt gdtr\n");
}
