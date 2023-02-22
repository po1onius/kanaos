#include <string.h>
#include <types.h>

int8 *strcpy(int8 *dest, const int8 *src)
{
    int8 *ptr = dest;
    while (true)
    {
        *ptr++ = *src;
        if (*src++ == EOS)
        {
            return dest;
        }
    }
}

int8 *strncpy(int8 *dest, const int8 *src, size_t count)
{
    int8 *ptr = dest;
    size_t nr = 0;
    for (; nr < count; nr++)
    {
        *ptr++ = *src;
        if (*src++ == EOS)
        {
            return dest;
        }
    }
    dest[count - 1] = EOS;
    return dest;
}

int8 *strcat(int8 *dest, const int8 *src)
{
    int8 *ptr = dest;
    while (*ptr != EOS)
    {
        ptr++;
    }
    while (true)
    {
        *ptr++ = *src;
        if (*src++ == EOS)
        {
            return dest;
        }
    }
}

size_t strlen(const int8 *str)
{
    int8 *ptr = (int8 *)str;
    while (*ptr != EOS)
    {
        ptr++;
    }
    return ptr - str;
}

int32 strcmp(const int8 *lhs, const int8 *rhs)
{
    while (*lhs == *rhs && *lhs != EOS && *rhs != EOS)
    {
        lhs++;
        rhs++;
    }
    return *lhs < *rhs ? -1 : *lhs > *rhs;
}

int8 *strchr(const int8 *str, int32 ch)
{
    int8 *ptr = (int8 *)str;
    while (true)
    {
        if (*ptr == ch)
        {
            return ptr;
        }
        if (*ptr++ == EOS)
        {
            return NULL;
        }
    }
}

int8 *strrchr(const int8 *str, int32 ch)
{
    int8 *last = NULL;
    int8 *ptr = (int8 *)str;
    while (true)
    {
        if (*ptr == ch)
        {
            last = ptr;
        }
        if (*ptr++ == EOS)
        {
            return last;
        }
    }
}

int32 memcmp(const void *lhs, const void *rhs, size_t count)
{
    int8 *lptr = (int8 *)lhs;
    int8 *rptr = (int8 *)rhs;
    while ((count > 0) && *lptr == *rptr)
    {
        lptr++;
        rptr++;
        count--;
    }
    if (count == 0)
    {
        return 0;
    }
    return *lptr < *rptr ? -1 : *lptr > *rptr;
}

void *memset(void *dest, int32 ch, size_t count)
{
    int8 *ptr = dest;
    while (count--)
    {
        *ptr++ = ch;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    int8 *ptr = dest;
    while (count--)
    {
        *ptr++ = *((int8 *)(src++));
    }
    return dest;
}

void *memchr(const void *str, int32 ch, size_t count)
{
    int8 *ptr = (int8 *)str;
    while (count--)
    {
        if (*ptr == ch)
        {
            return (void *)ptr;
        }
        ptr++;
    }
}


// 获取第一个分隔符
int8 *strsep(const int8 *str)
{
    int8 *ptr = (int8 *)str;
    while (true)
    {
        if (IS_SEPARATOR(*ptr))
        {
            return ptr;
        }
        if (*ptr++ == EOS)
        {
            return NULL;
        }
    }
}

// 获取最后一个分隔符
int8 *strrsep(const int8 *str)
{
    int8 *last = NULL;
    int8 *ptr = (int8 *)str;
    while (true)
    {
        if (IS_SEPARATOR(*ptr))
        {
            last = ptr;
        }
        if (*ptr++ == EOS)
        {
            return last;
        }
    }
}
