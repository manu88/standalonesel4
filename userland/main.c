
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

    printf_("Hello world\n");

    printf_("untypeds are from %zi to %zi\n", bootInfo->untyped.start , bootInfo->untyped.end);
    seL4_DebugDumpScheduler();

    unsigned long long total = 0;
    for (int i=0;i <CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS;i++)
    {
        seL4_UntypedDesc untyped = bootInfo->untypedList[i];
        if(untyped.sizeBits)
        {
            printf_("%i untyped size %zi k isdev:%u\n",i, (1 << untyped.sizeBits)/1024, untyped.isDevice);
            if(!untyped.isDevice)
            {
                total += (1 << untyped.sizeBits)/1024;
            }
        }
    }
    printf_("Total mem %zi k\n", total);
    while (1)
    {
        /* code */
    }
    
}