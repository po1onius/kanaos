#include <bitmap.h>
#include <string.h>
#include <assert.h>

void bitmap_make(struct bitmap_t *map, int8 *bits, uint32 length, uint32 offset)
{
    map->base = bits;
    map->len = length;
    map->offset = offset;
}

void bitmap_init(struct bitmap_t* btm)
{
    memset(btm->base, 0, btm->len);
}

bool bitmap_test(struct bitmap_t *map, uint32 index)
{
    assert(index >= map->offset);

    // 得到位图的索引
    uint32 idx = index - map->offset;

    // 位图数组中的字节
    uint32 bytes = idx / 8;

    // 该字节中的那一位
    uint8 bits = idx % 8;

    assert(bytes < map->len);

    // 返回那一位是否等于 1
    return (map->base[bytes] & (1 << bits));
}

bool bitmap_scan_check(struct bitmap_t* btm, uint32 bit_index)
{
    return (btm->base[bit_index / 8] & (1 << (bit_index % 8)));
}

uint32 bitmap_scan(struct bitmap_t* btm, uint32 count)
{
    uint32 idx_byte = 0;        // 用于记录空闲位所在的字节
/* 先逐字节比较,蛮力法 */
    while (idx_byte < btm->len)
    {
        bool brk = false;
        for(size_t i = 0; i < 8; i++)
        {
            if(!bitmap_scan_check(btm, idx_byte * 8 + i))
            {
                brk = true;
                break;
            }
        }
/* 1表示该位已分配,所以若为0xff,则表示该字节内已无空闲位,向下一字节继续找 */
        if(brk)
        {
            break;
        }
        idx_byte++;
    }

    assert(idx_byte < btm->len);
    if (idx_byte == btm->len)   // 若该内存池找不到可用空间
    {
        return -1;
    }

    /* 若在位图数组范围内的某字节内找到了空闲位，
     * 在该字节内逐位比对,返回空闲位的索引。*/
    int32 idx_bit = 0;
    /* 和btmp->bits[idx_byte]这个字节逐位对比 */
    while ((uint8)(1 << idx_bit) & btm->base[idx_byte])
    {
        idx_bit++;
    }

    int32 bit_idx_start = idx_byte * 8 + idx_bit;    // 空闲位在位图内的下标
    if (count == 1)
    {
        return bit_idx_start;
    }

    uint32 bit_left = (btm->len * 8 - bit_idx_start);   // 记录还有多少位可以判断
    uint32 next_bit = bit_idx_start + 1;
    uint32 n = 1;        // 用于记录找到的空闲位的个数

    bit_idx_start = -1;        // 先将其置为-1,若找不到连续的位就直接返回
    while (bit_left-- > 0)
    {
        if (!(bitmap_scan_check(btm, next_bit)))
        {         // 若next_bit为0
            n++;
        }
        else
        {
            n = 0;
        }
        if (count == n)
        {           // 若找到连续的cnt个空位
            bit_idx_start = next_bit - count + 1;
            break;
        }
        next_bit++;
    }
    return bit_idx_start;

}

void bitmap_set(struct bitmap_t* btm, uint32 bit_index, int8 value)
{
    assert((value == 0) || (value == 1));
    uint32 byte_idx = bit_index / 8;    // 向下取整用于索引数组下标
    uint32 bit_odd  = bit_index % 8;    // 取余用于索引数组内的位

/* 一般都会用个0x1这样的数对字节中的位操作,
 * 将1任意移动后再取反,或者先取反再移位,可用来对位置0操作。*/
    if (value)
    {               // 如果value为1
        btm->base[byte_idx] |= (1 << bit_odd);
    }
    else
    {                   // 若为0
        btm->base[byte_idx] &= ~(1 << bit_odd);
    }

}

