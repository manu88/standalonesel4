#pragma once

extern "C"
{
#include "printf.h"

#ifndef printf
#define printf(fmt, ...) printf_(fmt, ##__VA_ARGS__)
#endif


static inline void oops()
{
    float *f = NULL;
    *f = 42.f;
}

static inline void assert(int pred, const char* message)
{
    if(!pred)
    {
        printf_("%s\n", message);
        oops();
    }
}

static inline void *memset(void *s, int c, size_t len)
{
    unsigned char* p= (unsigned char*) s;
    while(len--)
    {
        *p++ = (unsigned char)c;
    }
    return s;
}

#ifndef NOT_REACHED
#define NOT_REACHED() assert(false, "This should NOT be reached")
#endif
}
