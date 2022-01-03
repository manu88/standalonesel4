#pragma once
#include "Platform.hpp"
#include "lib/expected.hpp"
#include "lib/vector.hpp"
#include "sel4.hpp"
#include <cstddef>

class InitialUntypedPool {
public:
  using ObjectOrError = Expected<seL4_CPtr, seL4_Error>;
  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;

  ObjectOrError allocObject(seL4_Word type);
  ObjectOrError allocObject(seL4_Word type, seL4_CNode root);
  void releaseObject(seL4_CPtr obj);

  SlotOrError getFreeSlot();
  void releaseSlot(seL4_SlotPos pos);
  void print();

private:
  seL4_SlotPos emptySlotPos = 0;
  vector<seL4_SlotPos> releasedSlots;

  size_t numAllocatedFromEmptyList = 0;
  size_t numAllocatedFromReleased = 0;
};