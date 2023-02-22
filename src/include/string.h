#ifndef __STRING_H__
#define __STRING_H__

#include <types.h>

#define SEPARATOR1 '/'                                       // 目录分隔符 1
#define SEPARATOR2 '\\'                                      // 目录分隔符 2
#define IS_SEPARATOR(c) (c == SEPARATOR1 || c == SEPARATOR2) // 字符是否位目录分隔符

int8 *strcpy(int8 *dest, const int8 *src);
int8 *strncpy(int8 *dest, const int8 *src, size_t count);
int8 *strcat(int8 *dest, const int8 *src);
size_t strlen(const int8 *str);
int32 strcmp(const int8 *lhs, const int8 *rhs);
int8 *strchr(const int8 *str, int32 ch);
int8 *strrchr(const int8 *str, int32 ch);
int8 *strsep(const int8 *str);
int8 *strrsep(const int8 *str);

int32 memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(void *dest, int32 ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
void *memchr(const void *ptr, int32 ch, size_t count);

#endif