#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <types.h>

void assertion_failure(int8 *exp, int8*file, int8*base, int32 line);

#define assert(exp) \
    if (exp)        \
        ;           \
    else            \
        assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)

void panic(const int8* fmt, ...);

#endif
