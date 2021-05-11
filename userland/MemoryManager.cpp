#include "MemoryManager.hpp"
#include "runtime.h"
#include "sel4.hpp"


/**
 * find a free untyped slot
 */
seL4_SlotPos find_untyped(seL4_SlotPos untyped_start, seL4_SlotPos untyped_end,
	const seL4_UntypedDesc* untyped_list, size_t needed_size)
{
	for(seL4_SlotPos cur_slot=untyped_start; cur_slot<untyped_end; ++cur_slot)
	{
		const seL4_UntypedDesc *cur_descr = untyped_list + (cur_slot-untyped_start);

		if(cur_descr->isDevice)
			continue;

		size_t cur_size = (1<< cur_descr->sizeBits);

		//printf("Untyped slot 0x%lx: size=%ld, physical address=0x%lx.\n",
		//	cur_slot, cur_size, cur_descr->paddr);
		if(cur_size < needed_size)
			continue;
		return cur_slot;
	}

	return 0;
}

MemoryManager::MemoryManager()
{
    printf("Init MemoryManager\n");

    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) GetBootInfo(); 
    assert(bootInfo != nullptr, "no bootinfo pointer for _start_root");
    size_t total = 0;
    
    InitialUntypedPool::instance().forEachNonDeviceRange([&](const auto range){
        total += range.size;
    });

    _availableNonDeviceUntypedMem = total;
    printf("Total non-device untypeds memory %zi Mib\n", total/(1024*1024));

    printf("Test alloc \n");
    printf("seL4_GetIPCBuffer is at %p\n", (void*) seL4_GetIPCBuffer());
    size_t numAlloc = 0;
    seL4_SlotPos cnodeIndex =  GetBootInfo()->empty.start;
    printf("1st empty slot is at %zi\n", cnodeIndex);

    unsigned tcbUntyped = InitialUntypedPool::instance().alloc(seL4_EndpointBits);
    printf("tcbUntyped=%u\n", tcbUntyped);

    const int ret = seL4_Untyped_Retype(tcbUntyped,
                                        seL4_EndpointObject,
                                        seL4_EndpointBits,
                                        seL4_CapInitThreadCNode,
                                        0,
                                        0,
                                        cnodeIndex,
                                        1
                                        );
    printf("seL4_Untyped_Retype returned %i\n", ret);
    while (true)
    {
        /* code */
    }
    
    while (true)
    {
        unsigned sel = InitialUntypedPool::instance().alloc(seL4_TCBBits);
        printf("%zi allocated page sel is %u\n",numAlloc, sel);

        if (InitialUntypedPool::instance().isExhausted())
        {
            printf("Initial pool exhausted at %zi\n", numAlloc);
            break;
        }
        printf("try to alloc at %zi\n", cnodeIndex);
		int          const node_index  = 0;
		int          const node_depth  = 0;
		int          const node_offset = cnodeIndex;
		int          const num_objects = 1;

		const int ret = seL4_Untyped_Retype(sel,
		                                    seL4_TCBObject,
		                                    seL4_TCBBits,
		                                    seL4_CapInitThreadCNode,
		                                    node_index,
		                                    node_depth,
		                                    node_offset,
		                                    num_objects);
        printf("seL4_Untyped_Retype returned %i\n", ret);
        numAlloc++;
    }
    

}