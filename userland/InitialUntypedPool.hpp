#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"
#include "lib/expected.hpp"
#include "Platform.hpp"



class InitialUntypedPool
{
public:
    using ObjectOrError = Expected<seL4_CPtr, seL4_Error>;
    using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;

    ObjectOrError allocObject(seL4_Word type);

    SlotOrError getFreeSlot();
    void releaseSlot(seL4_SlotPos pos);

private:
    seL4_SlotPos emptySlotPos = 0;
};