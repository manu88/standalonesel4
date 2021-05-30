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
    assert(GetBootInfo() != nullptr, "InitialUntypedPool::forEachRange: null bootinfo");
	for (unsigned sel = GetBootInfo()->untyped.start; sel < GetBootInfo()->untyped.end; sel++) 
    {
        UntypedRange r(*this, sel);

        func(r);
    }

}

seL4_Word InitialUntypedPool::_align_offset(UntypedRange &range, size_t size_log2)
{
    /*
        * The seL4 kernel naturally aligns allocations within untuped
        * memory ranges. So we have to apply the same policy to our
        * shadow version of the kernel's 'FreeIndex'.
        */
    seL4_Word const aligned_free_offset = align_addr(range.freeOffset,
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
        assert(range.isDevice == false, "untyped should not be device!");
        /* calculate free index after allocation */
        seL4_Word const newFreeOffset = _align_offset(range, size_log2);
        /* check if allocation fits within current untyped memory range */
        if (newFreeOffset > range.size)
            return;

        if (sel == UNKNOWN) {
            sel = range.sel;
            return;
        }

        /* check which range is smaller - take that */
        seL4_Word const rest = range.size - newFreeOffset;

        UntypedRange bestFit(*this, sel);
        seL4_Word const newFreeOffsetBest = _align_offset(bestFit, size_log2);
        seL4_Word const restBest = bestFit.size - newFreeOffsetBest;

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
    seL4_Word const newFreeOffset = _align_offset(bestFit, size_log2);
    assert(newFreeOffset <= bestFit.size, "newFreeOffset <= best_fit.size");

    /*
    * We found a matching range, consume 'size' and report the
    * selector. The returned selector is used by the caller
    * of 'alloc' to perform the actual kernel-object creation.
    */
    bestFit.freeOffset = newFreeOffset;
    printf("Alloc sel is at %u\n", bestFit.sel);
    assert(bestFit.isDevice == false, "alloc: returned untyped should NOT be a device");

    return bestFit.sel;
}


/**
 * find a free untyped slot
 */
seL4_SlotPos find_untyped(seL4_SlotPos untyped_start, seL4_SlotPos untyped_end,
	const seL4_UntypedDesc* untyped_list, seL4_Word needed_size)
{
	for(seL4_SlotPos cur_slot=untyped_start; cur_slot<untyped_end; ++cur_slot)
	{
		const seL4_UntypedDesc *cur_descr = untyped_list + (cur_slot-untyped_start);

		if(cur_descr->isDevice)
			continue;

		seL4_Word cur_size = (1<< cur_descr->sizeBits);

		//printf("Untyped slot 0x%lx: size=%ld, physical address=0x%lx.\n",
		//	cur_slot, cur_size, cur_descr->paddr);
		if(cur_size < needed_size)
			continue;
		return cur_slot;
	}

	return 0;
}

seL4_SlotPos get_slot(seL4_Word obj, seL4_Word obj_size,
	seL4_SlotPos untyped_start, seL4_SlotPos untyped_end, const seL4_UntypedDesc* untyped_list,
	seL4_SlotPos* cur_slot, seL4_SlotPos cnode)
{
	seL4_SlotPos slot = find_untyped(untyped_start, untyped_end, untyped_list, obj_size);
	printf("Found untyped at %zi\n", slot);
    seL4_SlotPos offs = (*cur_slot)++;
	seL4_Error err =  seL4_Untyped_Retype(slot, obj, 0, cnode, 0, 0, offs, 1);
    if(err != seL4_NoError)
    {
        return err;
    }

	return offs;
}

seL4_SlotPos InitialUntypedPool::getSlot(seL4_Word obj, seL4_Word size)
{
    auto bi = GetBootInfo();
    return get_slot(obj, 1 << size, bi->untyped.start, bi->untyped.end, bi->untypedList, &currentSlot, seL4_CapInitThreadCNode);
}

void map_pagetables(seL4_SlotPos untyped_start, seL4_SlotPos untyped_end, const seL4_UntypedDesc* untyped_list, seL4_SlotPos* cur_slot, seL4_Word virt_addr)
{
	const seL4_SlotPos cnode = seL4_CapInitThreadCNode;
	const seL4_SlotPos vspace = seL4_CapInitThreadVSpace;
	seL4_X86_VMAttributes vmattr = seL4_X86_Default_VMAttributes;

	seL4_SlotPos table_slot = find_untyped(untyped_start, untyped_end, untyped_list, PAGE_SIZE*1024);
	if(table_slot < untyped_start)
    {
		printf("Error: No large enough untyped slot found!\n");
    }
	printf("Loading tables into untyped slot 0x%lx.\n", table_slot);

	// load three levels of page tables
	const seL4_Word pagetable_objs[] =
	{
		seL4_X86_PDPTObject,
		seL4_X86_PageDirectoryObject,
		seL4_X86_PageTableObject
	};

	seL4_Error (*pagetable_map[])(seL4_Word, seL4_CPtr, seL4_Word, seL4_X86_VMAttributes) =
	{
		&seL4_X86_PDPT_Map,
		&seL4_X86_PageDirectory_Map,
		&seL4_X86_PageTable_Map
	};

	seL4_SlotPos pagetable_slots[] = { 0, 0, 0 };

	for(size_t level=0; level<sizeof(pagetable_objs)/sizeof(pagetable_objs[0]); ++level)
	{
		pagetable_slots[level] = (*cur_slot)++;
        seL4_Error err = seL4_Untyped_Retype(table_slot, pagetable_objs[level], 0, cnode, 0, 0, pagetable_slots[level], 1);
        if(err != seL4_NoError)
		{
			printf("seL4_Untyped_Retype Error mapping page table level %d!: %i\n", level, err);
			break;
		}

        err = (*pagetable_map[level])(pagetable_slots[level], vspace, virt_addr, vmattr); 
		if(err != seL4_NoError)
		{
			printf("Error mapping page table level %d!: %i\n", level, err);
			break;
		}
	}
}

void InitialUntypedPool::mapPageTables(seL4_Word virtualAddress)
{
    auto bi = GetBootInfo();
    map_pagetables(bi->untyped.start, bi->untyped.end, bi->untypedList, &currentSlot, virtualAddress);
}