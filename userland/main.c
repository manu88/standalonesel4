

#include "MemoryManager.hpp"
#include "sel4/sel4_arch/syscalls.h"

extern "C"
{

#include "runtime.h"
static int a = 0;

__attribute__ ((constructor)) void foo(void)
{
    a = 10;
}

void start_root()
{
    printf("Hello world a=%i\n", a);

    seL4_DebugDumpScheduler();

    MemoryManager memManager;
    while (1)
    {
        /* code */
    }
}
}