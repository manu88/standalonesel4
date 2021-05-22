#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"


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
    // Wrapper around seL4_UntypedDesc
    struct UntypedRange
    {
        UntypedRange(InitialUntypedPool&, size_t sel);

        /* core-local cap selector */
        size_t sel;
        
        /* index into 'untypedSizeBitsList' */
        size_t index = sel - seL4_GetBootInfo()->untyped.start; 
        bool isDevice = seL4_GetBootInfo()->untypedList[sel].isDevice;

        /* original size of untyped memory range */
        size_t size = 1UL << seL4_GetBootInfo()->untypedList[sel].sizeBits;
                
        /* physical address of the begin of the untyped memory range */
        addr_t physAddress = seL4_GetBootInfo()->untypedList[sel].paddr;

        /* offset to the unused part of the untyped memory range */
        addr_t &freeOffset;
    };

    using UntypedRangeVisitor = std::function<void(UntypedRange&)>;
    void forEachRange(const UntypedRangeVisitor&);
    void forEachNonDeviceRange(const UntypedRangeVisitor&);

    /**
     * Return selector of untyped memory range where the allocation of
     * the specified size is possible
     *
     * \param kernel object size
     *
     * This function models seL4's allocation policy of untyped memory. It
     * is solely used at boot time to setup core's initial kernel objects
     * from the initial pool of untyped memory ranges as reported by the
     * kernel.
     *
s     */
    unsigned alloc(size_t size_log2);


    bool isExhausted() const {
        return _isExhausted;
    }

private:
    InitialUntypedPool(){}

    enum { MAX_UNTYPED = (size_t)CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS };

    struct Free_offset { addr_t value = 0; };

    Free_offset _free_offset[MAX_UNTYPED];

    /**
     * Calculate free index after allocation
     */
    addr_t _align_offset(UntypedRange &range, size_t size_log2);


    bool _isExhausted = false;

};