#pragma once
#include <cstddef>
#include "InitialUntypedPool.hpp"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

class MemoryManager
{
public:
    void init();

    enum VirtualAddressLayout
    {
        AddressTables = 0x8000000000
    };

    seL4_Error mapPage(seL4_Word vaddr, seL4_CapRights_t rights);

private:
    seL4_CPtr pdpt = 0;
    seL4_CPtr pd = 0;
    seL4_CPtr pt = 0;
    
};