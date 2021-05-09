
#include "runtime.h"
#include "sel4/bootinfo.h"
#define NULL 0

void _putchar(char c)
{
    seL4_DebugPutChar(c);
}

static int a = 0;


__attribute__ ((constructor)) void foo(void)
{
    a = 1;
    oops();
}




void __sel4_start_root(void* bootinfoPtr)
{
    assert(bootinfoPtr, "no bootinfo pointer for __sel4_start_root");
    
    seL4_BootInfo* bootInfo = bootinfoPtr; 

    printf_("Hello world\n\n");
    seL4_DebugDumpScheduler();
    while (1)
    {
        /* code */
    }
    
}