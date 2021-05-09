
#include "runtime.h"
#include "sel4/bootinfo.h"
#define NULL 0

void __sel4_start_root(void* bootinfoPtr)
{
    assert(bootinfoPtr, "no bootinfo pointer for __sel4_start_root");
    
    seL4_BootInfo* bootInfo = bootinfoPtr; 

    printf("Hello world\n");

    printf("untypeds are from %zi to %zi\n", bootInfo->untyped.start , bootInfo->untyped.end);
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
    printf("Total mem %zi m\n", total/1024);
    while (1)
    {
        /* code */
    }
}