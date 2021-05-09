#pragma once

extern "C"
{
#include "printf.h"
#include "sel4/sel4_arch/syscalls.h"

#define NULL 0

#define printf(fmt, ...) printf_(fmt, ##__VA_ARGS__)

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

}