#include "MemoryManager.hpp"
#include "runtime.h"
extern "C"
{ 
#include "sel4/bootinfo.h"
}

MemoryManager::MemoryManager()
{
    printf("Init MemoryManager\n");

    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo(); 
    assert(bootInfo != nullptr, "no bootinfo pointer for _start_root");
    
    size_t total = 0;
    for (int i=0;i <CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS;i++)
    {
        seL4_UntypedDesc untyped = bootInfo->untypedList[i];
        if(untyped.sizeBits)
        {
            if(!untyped.isDevice)
            {
                total += (1 << untyped.sizeBits);
            }
        }
    }
    _availableNonDeviceUntypedMem = total;
    printf("Total non-device untypeds memory %zi Mib\n", total/(1024*1024));


}