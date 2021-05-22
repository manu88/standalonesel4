#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"


InitialUntypedPool::UntypedRange::UntypedRange(InitialUntypedPool& pool, size_t sel):
sel(sel),
freeOffset(pool._free_offset[index].value)
{

}

void InitialUntypedPool::forEachNonDeviceRange(const InitialUntypedPool::UntypedRangeVisitor &func)
{
    forEachRange([&](UntypedRange&r){
        if(!r.isDevice)
        {
            func(r);
        }
    });
}

void InitialUntypedPool::forEachRange(const InitialUntypedPool::UntypedRangeVisitor &func)
{
    assert(seL4_GetBootInfo() != nullptr, "InitialUntypedPool::forEachRange: null bootinfo");
	for (unsigned sel = seL4_GetBootInfo()->untyped.start; sel < seL4_GetBootInfo()->untyped.end; sel++) 
    {
        UntypedRange r(*this, sel);

        func(r);
    }

}

addr_t InitialUntypedPool::_align_offset(UntypedRange &range, size_t size_log2)
{
    /*
        * The seL4 kernel naturally aligns allocations within untuped
        * memory ranges. So we have to apply the same policy to our
        * shadow version of the kernel's 'FreeIndex'.
        */
    addr_t const aligned_free_offset = align_addr(range.freeOffset,
                                                    size_log2);

    return aligned_free_offset + (1 << size_log2);
}


unsigned InitialUntypedPool::alloc(size_t size_log2)
{
    enum { UNKNOWN = 0 };
    unsigned sel = UNKNOWN;

    /*
    * Go through the known initial untyped memory ranges to find
    * a range that is able to host a kernel object of 'size'.
    */
    forEachNonDeviceRange([&] (UntypedRange &range)
    {
        /* calculate free index after allocation */
        addr_t const newFreeOffset = _align_offset(range, size_log2);
        /* check if allocation fits within current untyped memory range */
        if (newFreeOffset > range.size)
            return;

        if (sel == UNKNOWN) {
            sel = range.sel;
            return;
        }

        /* check which range is smaller - take that */
        addr_t const rest = range.size - newFreeOffset;

        UntypedRange bestFit(*this, sel);
        addr_t const newFreeOffsetBest = _align_offset(bestFit, size_log2);
        addr_t const restBest = bestFit.size - newFreeOffsetBest;

        if (restBest >= rest)
            /* current range fits better then best range */
            sel = range.sel;
    });

    if (sel == UNKNOWN) 
    {
        printf("Initial_untyped_pool exhausted\n");
        _isExhausted = true;
        return 0;
    }

    UntypedRange bestFit(*this, sel);
    addr_t const newFreeOffset = _align_offset(bestFit, size_log2);
    assert(newFreeOffset <= bestFit.size, "newFreeOffset <= best_fit.size");

    /*
    * We found a matching range, consume 'size' and report the
    * selector. The returned selector is used by the caller
    * of 'alloc' to perform the actual kernel-object creation.
    */
    bestFit.freeOffset = newFreeOffset;

    return bestFit.sel;
}