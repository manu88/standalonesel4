#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"


seL4_CPtr alloc_slot(seL4_BootInfo *info)
{
    if(info->empty.start == info->empty.end)
    {
        printf("No CSlots left!\n");
        return 0;
    }
    seL4_CPtr next_free_slot = info->empty.start++;
    return next_free_slot;
}



/* a very simple allocation function that iterates through the untypeds in boot info until
   a retype succeeds */
seL4_CPtr alloc_object(seL4_BootInfo *info, seL4_Word type)
{
    seL4_CPtr cslot = alloc_slot(info);

    /* keep trying to retype until we succeed */
    seL4_Error error = seL4_NotEnoughMemory;
    for (seL4_CPtr untyped = info->untyped.start; untyped < info->untyped.end; untyped++) {
        seL4_UntypedDesc *desc = &info->untypedList[untyped - info->untyped.start];
        if (!desc->isDevice) 
        {
            seL4_Error error = seL4_Untyped_Retype(untyped, type, 0, seL4_CapInitThreadCNode, 0, 0, cslot, 1);
            if (error == seL4_NoError) 
            {
                return cslot;
            } 
            else if (error != seL4_NotEnoughMemory) 
            {
                printf("Failed to allocate untyped\n");
            }
        }
    }
    if(error == seL4_NotEnoughMemory)
    {
        printf("Out of untyped memory\n");
    }
    return cslot;
}

seL4_CPtr InitialUntypedPool::allocObject(seL4_Word type)
{
    return alloc_object(GetBootInfo(), type);
}

