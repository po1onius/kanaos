#ifndef __STDLIB_H__
#define __STDLIB_H__

#include <types.h>

#define MAX(a, b) (a < b ? b : a)
#define MIN(a, b) (a < b ? a : b)

uint8 bcd_to_bin(uint8 value);
uint8 bin_to_bcd(uint8 value);
int32 atoi(const int8 *str);
#endif