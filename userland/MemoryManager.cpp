#include "MemoryManager.hpp"
#include "runtime.h"
#include "sel4.hpp"

MemoryManager::MemoryManager()
{
    printf("Init MemoryManager\n");

    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo(); 
    assert(bootInfo != nullptr, "no bootinfo pointer for _start_root");
    size_t total = 0;
    
    _untypeds.forEachNonDeviceRange([&](const auto range){
        total += range.size;
    });

    _availableNonDeviceUntypedMem = total;
    printf("Total non-device untypeds memory %zi Mib\n", total/(1024*1024));
}