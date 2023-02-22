#include <stdlib.h>
#include <types.h>

// 将 bcd 码转成整数
uint8 bcd_to_bin(uint8 value)
{
    return (value & 0xf) + (value >> 4) * 10;
}

// 将整数转成 bcd 码
uint8 bin_to_bcd(uint8 value)
{
    return (value / 10) * 0x10 + (value % 10);
}

int32 atoi(const int8 *str)
{
    if (str == NULL)
    {
        return 0;
    }
    bool sign = 1;
    int32 result = 0;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    for (; *str; str++)
    {
        result = result * 10 + (*str - '0');
    }
    return result;
}