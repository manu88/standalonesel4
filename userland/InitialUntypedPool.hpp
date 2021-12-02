#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"

#define PAGE_TYPE        seL4_X86_4K
#define PAGE_SIZE        4096

class InitialUntypedPool
{
public:
    static auto& instance()
    {
        static InitialUntypedPool _instance;
        return _instance;
    }

    seL4_SlotPos getSlot(seL4_Word obj, seL4_Word size);

    seL4_CPtr allocObject(seL4_Word type);

private:
    InitialUntypedPool(){}
};