#ifndef __TYPES_H__
#define __TYPES_H__

#define EOF -1
#define EOS '\0'
#define bool _Bool
#define NULL 0
#define true 1
#define false 0
#define _packed __attribute((packed))
#define _inline __attribute__((always_inline)) inline
#define _ofp __attribute__((optimize("omit-frame-pointer")))

#define CONCAT(x, y) x##y
#define RESERVED_TOKEN(x, y) CONCAT(x, y)
#define RESERVED RESERVED_TOKEN(reserved, __LINE__)

typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef uint32 size_t;

typedef uint32 time_t;

struct selector
{
    uint8 RPL : 2;//选择子特权级
    uint8 TI : 1;//全局/本地 描述符表
    uint16 index : 13;//表索引
};

struct dt_ptr
{
    uint16 limit;//表的范围
    uint32 base;//表的地址
}_packed;

enum std_fd_t
{
    stdin,
    stdout,
    stderr,
};


#endif