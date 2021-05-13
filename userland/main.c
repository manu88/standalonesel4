
#include <sel4/sel4.h>

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
    //oops();
}


void __sel4_start_root()
{
    seL4_GetBootInfo();
    while (1)
    {
        /* code */
    }
    
}