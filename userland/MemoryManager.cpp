#include "MemoryManager.hpp"
#include "runtime.h"
#include "sel4.hpp"

MemoryManager::MemoryManager()
{
    printf("Init MemoryManager\n");

    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo(); 
    assert(bootInfo != nullptr, "no bootinfo pointer for _start_root");
    size_t total = 0;
    
    InitialUntypedPool::instance().forEachNonDeviceRange([&](const auto range){
        total += range.size;
    });

    _availableNonDeviceUntypedMem = total;
    printf("Total non-device untypeds memory %zi Mib\n", total/(1024*1024));

    printf("Test alloc \n");

    unsigned sel = InitialUntypedPool::instance().alloc(13);
    printf("allocated page sel is %u\n", sel);
}