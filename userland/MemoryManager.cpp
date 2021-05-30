#include "MemoryManager.hpp"
#include "runtime.h"
#include "sel4.hpp"


MemoryManager::MemoryManager()
{
#if 0
   

    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) GetBootInfo(); 
    assert(bootInfo != nullptr, "no bootinfo pointer for _start_root");
    size_t total = 0;
    
    InitialUntypedPool::instance().forEachNonDeviceRange([&](const auto range){
        total += range.size;
    });
    _availableNonDeviceUntypedMem = total;
    printf("Total non-device untypeds memory %zi Mib\n", total/(1024*1024));


    printf("Test alloc \n");
    size_t numAlloc = 0;
    seL4_SlotPos cnodeIndex = GetBootInfo()->empty.start;
    while (true)
    {
        unsigned sel = InitialUntypedPool::instance().alloc(seL4_PageBits);
        printf("%zi allocated page sel is %u\n",numAlloc, sel);

        if (InitialUntypedPool::instance().isExhausted())
        {
            printf("Initial pool exhausted at %zi\n", numAlloc);
            break;
        }

		int          const node_index  = 0;
		int          const node_depth  = 0;
		int          const node_offset = cnodeIndex + numAlloc;
		int          const num_objects = 1;

		int const ret = seL4_Untyped_Retype(sel,
		                                    seL4_X86_4K,
		                                    seL4_PageBits,
		                                    seL4_CapInitThreadCNode,
		                                    node_index,
		                                    node_depth,
		                                    node_offset,
		                                    num_objects);
        if (ret == 0)
            numAlloc++;
    }
    
#endif
}

void MemoryManager::init()
{
    printf("MemoryManager: init\n");
    InitialUntypedPool::instance().mapPageTables(MemoryManager::VirtualAddressLayout::AddressTables);
    printf("MemoryManager: Page table mapped\n");
}


