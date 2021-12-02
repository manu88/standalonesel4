#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"

InitialUntypedPool::InitialUntypedPool()
{
}

static seL4_CPtr allocSlot(seL4_BootInfo *info)
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
InitialUntypedPool::ObjectOrError InitialUntypedPool::allocObject(seL4_Word type)
{
    auto info = seL4::GetBootInfo();
    seL4_CPtr cslot = allocSlot(info);

    /* keep trying to retype until we succeed */
    seL4_Error error = seL4_NotEnoughMemory;
    for (seL4_CPtr untyped = info->untyped.start; untyped < info->untyped.end; untyped++) {
        seL4_UntypedDesc *desc = &info->untypedList[untyped - info->untyped.start];
        if (!desc->isDevice) 
        {
            seL4_Error error = seL4_Untyped_Retype(untyped, type, 0, seL4_CapInitThreadCNode, 0, 0, cslot, 1);
            if (error == seL4_NoError) 
            {
                return success<seL4_CPtr, seL4_Error>(cslot);
            }
        }
    }
    if(error != seL4_NoError)
    {
        return unexpected<seL4_CPtr, seL4_Error>(error);
    }
    return success<seL4_CPtr, seL4_Error>(cslot);
}

InitialUntypedPool::SlotOrError InitialUntypedPool::getSlot()
{
    if(emptySlotPos == 0)
    {
        emptySlotPos = getEmptySlotRegion().start;
    }
    else if(emptySlotPos > getEmptySlotRegion().end)
    {
        return unexpected<seL4_SlotPos, seL4_Error>(seL4_NotEnoughMemory);
    }
    seL4_SlotPos ret = emptySlotPos;
    emptySlotPos++;
    return success<seL4_SlotPos, seL4_Error>(ret);
}