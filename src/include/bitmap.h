#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <types.h>

struct bitmap_t
{
    int8* base;
    uint32 len;
    uint32 offset;
};
void bitmap_make(struct bitmap_t *map, int8 *bits, uint32 length, uint32 offset);
bool bitmap_test(struct bitmap_t *map, uint32 index);
void bitmap_init(struct bitmap_t* btm);
bool bitmap_scan_check(struct bitmap_t* btm, uint32 bit_index);
uint32 bitmap_scan(struct bitmap_t* btm, uint32 count);
void bitmap_set(struct bitmap_t* btm, uint32 bit_index, int8 value);

#endif
