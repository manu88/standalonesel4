#pragma once

#include "sel4/sel4_arch/syscalls.h"

#define NULL 0


static inline void print(const char* str)
{
    char* c = (char*) str;
    while (*c) seL4_DebugPutChar(*c++);
}



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

