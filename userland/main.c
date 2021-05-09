#include "sel4/syscalls.h"

#define NULL 0
static int a = 0;

static void oops()
{
    float *f = NULL;
    *f = 42.f;
}

static void oopsIfNull(void* p)
{
    if(!p)
    {
        oops();
    }
}

__attribute__ ((constructor)) void foo(void)
{
    a = 1;
    oops();
}


void print(const char* str)
{
    while (str++)
    {
        seL4_DebugPutChar(str);
    }
    
}
void __sel4_start_root(void* bootinfo)
{
    oopsIfNull(bootinfo);
    
    while (1)
    {
        /* code */
    }
    
}