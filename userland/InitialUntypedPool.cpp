#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"

/* a very simple allocation function that iterates through the untypeds in boot
   info until a retype succeeds */
InitialUntypedPool::ObjectOrError
InitialUntypedPool::allocObject(seL4_Word type) {
  auto info = seL4::GetBootInfo();
  auto slotOrErr = getFreeSlot();
  if (!slotOrErr) {
    return unexpected<seL4_CPtr, seL4_Error>(slotOrErr.error);
  }
  seL4_CPtr cslot = slotOrErr.value;

  /* keep trying to retype until we succeed */
  seL4_Error error = seL4_NotEnoughMemory;
  for (seL4_CPtr untyped = info->untyped.start; untyped < info->untyped.end;
       untyped++) {
    seL4_UntypedDesc *desc = &info->untypedList[untyped - info->untyped.start];
    if (!desc->isDevice) {
      seL4_Error error = seL4_Untyped_Retype(
          untyped, type, 0, seL4_CapInitThreadCNode, 0, 0, cslot, 1);
      if (error == seL4_NoError) {
        return success<seL4_CPtr, seL4_Error>(cslot);
      }
    }
  }
  if (error != seL4_NoError) {
    return unexpected<seL4_CPtr, seL4_Error>(error);
  }
  return success<seL4_CPtr, seL4_Error>(cslot);
}

void InitialUntypedPool::releaseObject(seL4_CPtr) {
  kprintf("InitialUntypedPool::releaseObject does nothing right now :)\n");
}

InitialUntypedPool::SlotOrError InitialUntypedPool::getFreeSlot() {
  if (releasedSlots.size() > 0) {
    seL4_SlotPos slot = releasedSlots.back();
    releasedSlots.pop_back();
    return success<seL4_SlotPos, seL4_Error>(slot);
  }
  auto empty = seL4::GetBootInfo()->empty;
  if (emptySlotPos == 0) {
    emptySlotPos = empty.start;
  } else if (emptySlotPos > empty.end) {
    return unexpected<seL4_SlotPos, seL4_Error>(seL4_NotEnoughMemory);
  }
  seL4_SlotPos ret = emptySlotPos;
  emptySlotPos++;
  return success<seL4_SlotPos, seL4_Error>(ret);
}

void InitialUntypedPool::releaseSlot(seL4_SlotPos slot) {
  releasedSlots.push_back(slot);
}