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
        if(r.isDevice)
        {
            return;
        }
        func(r);
    });
}

void InitialUntypedPool::forEachRange(const InitialUntypedPool::UntypedRangeVisitor &func)
{
    assert(GetBootInfo() != nullptr, "InitialUntypedPool::forEachRange: null bootinfo");
	for (unsigned sel = GetBootInfo()->untyped.start; sel < GetBootInfo()->untyped.end; sel++) 
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
        addr_t const new_free_offset = _align_offset(range, size_log2);
        /* check if allocation fits within current untyped memory range */
        if (new_free_offset > range.size)
            return;

        if (sel == UNKNOWN) {
            sel = range.sel;
            return;
        }

        /* check which range is smaller - take that */
        addr_t const rest = range.size - new_free_offset;

        UntypedRange best_fit(*this, sel);
        addr_t const new_free_offset_best = _align_offset(best_fit, size_log2);
        addr_t const rest_best = best_fit.size - new_free_offset_best;

        if (rest_best >= rest)
            /* current range fits better then best range */
            sel = range.sel;
    });

    if (sel == UNKNOWN) 
    {
        printf("Initial_untyped_pool exhausted\n");
        _isExhausted = true;
        return 0;
    }

    UntypedRange best_fit(*this, sel);
    addr_t const new_free_offset = _align_offset(best_fit, size_log2);
    assert(new_free_offset <= best_fit.size, "new_free_offset <= best_fit.size");

    /*
    * We found a matching range, consume 'size' and report the
    * selector. The returned selector is used by the caller
    * of 'alloc' to perform the actual kernel-object creation.
    */
    best_fit.freeOffset = new_free_offset;

    return best_fit.sel;
}