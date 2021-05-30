#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"

#define PAGE_TYPE        seL4_X86_4K
#define PAGE_SIZE        4096

/*
Heavily inspired from Genode:
https://github.com/genodelabs/genode/blob/master/repos/base-sel4/
*/

/*
    * Alignment to the power of two
    */
template <typename T>
static constexpr T _align_mask(T align) 
{
    return ~(((T)1 << align) - (T)1);
}

template <typename T>
static constexpr T _align_offset(T align)
{
    return   ((T)1 << align) - (T)1;
}

template <typename T>
static constexpr T align_addr(T addr, int align)
{
    return (addr + _align_offset((T)align)) & _align_mask((T)align);
}

class InitialUntypedPool
{
public:
    static auto& instance()
    {
        static InitialUntypedPool _instance;
        return _instance;
    }

    seL4_SlotPos getSlot(seL4_Word obj, seL4_Word size);
    void mapPageTables(seL4_Word virtualAddress);

private:
    InitialUntypedPool(){}

    seL4_SlotPos getCurrentSlot() const 
    {
        return _currentSlot;
    }
    
    seL4_SlotPos getCurrentSlotAndAdvance() 
    {
        return _currentSlot++;
    }

    seL4_SlotPos _currentSlot = GetBootInfo()->empty.start;
};