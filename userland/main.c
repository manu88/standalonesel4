

#include "class.hpp"
#define NULL 0

extern "C"
{

#include "runtime.h"
#include "sel4/bootinfo.h"
static int a = 0;

__attribute__ ((constructor)) void foo(void)
{
    a = 10;
}

void start_root()
{
    assert(seL4_GetBootInfo() != nullptr, "no bootinfo pointer for _start_root");
    
    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo(); 

    printf("Hello world a=%i\n", a);

    seL4_DebugDumpScheduler();

    unsigned long long total = 0;
    for (int i=0;i <CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS;i++)
    {
        seL4_UntypedDesc untyped = bootInfo->untypedList[i];
        if(untyped.sizeBits)
        {
            if(!untyped.isDevice)
            {
                total += (1 << untyped.sizeBits)/1024;
            }
        }
    }
    printf("Total mem %zi M\n", total/1024);
    while (1)
    {
        /* code */
    }
}
}