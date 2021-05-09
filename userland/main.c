#include "sel4/sel4_arch/syscalls.h"
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
    char* c = str;
    while (*c) seL4_DebugPutChar(*c++);
}

void __sel4_start_root(void* bootinfo)
{
    oopsIfNull(bootinfo);

    print("Hello world\n\n");
    seL4_DebugDumpScheduler();
    while (1)
    {
        /* code */
    }
    
}