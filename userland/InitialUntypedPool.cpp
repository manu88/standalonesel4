#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"


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

		if(cur_size < needed_size)
			continue;
		return cur_slot;
	}

	return 0;
}

seL4_SlotPos InitialUntypedPool::getSlot(seL4_Word obj, seL4_Word size)
{
    auto bi = GetBootInfo();
    const size_t sizeInBits = 1 << size;
    seL4_SlotPos slot = find_untyped(bi->untyped.start, bi->untyped.end, bi->untypedList, sizeInBits);

    seL4_SlotPos offs = getCurrentSlotAndAdvance();
    seL4_Error err =  seL4_Untyped_Retype(slot, obj, 0, seL4_CapInitThreadCNode, 0, 0, offs, 1);

    if(err != seL4_NoError)
    {
        return err;
    }

	return offs;
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

	seL4_SlotPos pagetable_slots[] = { 0, 0, 0 };

// Level 0
    printf("Start loop level=%i slot is %zu\n", 0, *cur_slot);
    pagetable_slots[0] = (*cur_slot)++;
    seL4_Error err = seL4_Untyped_Retype(table_slot, seL4_X86_PDPTObject, 0, cnode, 0, 0, pagetable_slots[0], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level 0!: %i\n", err);
        return;
    }
    err = seL4_X86_PDPT_Map(pagetable_slots[0], vspace, virt_addr, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level 0!: %i\n", err);
        return;
    }

// Level 1
    printf("Start loop level=%i slot is %zu\n", 1, *cur_slot);
    pagetable_slots[1] = (*cur_slot)++;
    err = seL4_Untyped_Retype(table_slot, seL4_X86_PageDirectoryObject, 0, cnode, 0, 0, pagetable_slots[1], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level %d!: %i\n", 1, err);
        return;
    }

    err = seL4_X86_PageDirectory_Map(pagetable_slots[1], vspace, virt_addr, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level %d!: %i\n", 1, err);
        return;
    }
// Level 2
    printf("Start loop level=%i slot is %zu\n", 2, *cur_slot);
    pagetable_slots[2] = (*cur_slot)++;
    err = seL4_Untyped_Retype(table_slot, seL4_X86_PageTableObject, 0, cnode, 0, 0, pagetable_slots[2], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level %d!: %i\n", 2, err);
        return;
    }

    err = seL4_X86_PageTable_Map(pagetable_slots[2], vspace, virt_addr, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level %d!: %i\n", 2, err);
        return;
    }

}

void InitialUntypedPool::mapPageTables(seL4_Word virtualAddress)
{
    auto bi = GetBootInfo();
    const seL4_SlotPos cnode = seL4_CapInitThreadCNode;
	const seL4_SlotPos vspace = seL4_CapInitThreadVSpace;
	seL4_X86_VMAttributes vmattr = seL4_X86_Default_VMAttributes;

	seL4_SlotPos table_slot = find_untyped(bi->untyped.start, bi->untyped.end, bi->untypedList, PAGE_SIZE*1024);
	if(table_slot < bi->untyped.start)
    {
		printf("Error: No large enough untyped slot found!\n");
    }
	printf("Loading tables into untyped slot 0x%lx.\n", table_slot);

	seL4_SlotPos pagetable_slots[] = { 0, 0, 0 };

// Level 0
    pagetable_slots[0] = getCurrentSlotAndAdvance();
    printf("Start loop level=%i slot is %zu\n", 0, pagetable_slots[0]);
    seL4_Error err = seL4_Untyped_Retype(table_slot, seL4_X86_PDPTObject, 0, cnode, 0, 0, pagetable_slots[0], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level 0!: %i\n", err);
        return;
    }
    err = seL4_X86_PDPT_Map(pagetable_slots[0], vspace, virtualAddress, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level 0!: %i\n", err);
        return;
    }

// Level 1
    pagetable_slots[1] = getCurrentSlotAndAdvance();
    printf("Start loop level=%i slot is %zu\n", 0, pagetable_slots[1]);
    err = seL4_Untyped_Retype(table_slot, seL4_X86_PageDirectoryObject, 0, cnode, 0, 0, pagetable_slots[1], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level %d!: %i\n", 1, err);
        return;
    }

    err = seL4_X86_PageDirectory_Map(pagetable_slots[1], vspace, virtualAddress, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level %d!: %i\n", 1, err);
        return;
    }
// Level 2
    pagetable_slots[2] = getCurrentSlotAndAdvance();
    printf("Start loop level=%i slot is %zu\n", 0, pagetable_slots[2]);
    err = seL4_Untyped_Retype(table_slot, seL4_X86_PageTableObject, 0, cnode, 0, 0, pagetable_slots[2], 1);
    if(err != seL4_NoError)
    {
        printf("seL4_Untyped_Retype Error mapping page table level %d!: %i\n", 2, err);
        return;
    }

    err = seL4_X86_PageTable_Map(pagetable_slots[2], vspace, virtualAddress, vmattr); 
    if(err != seL4_NoError)
    {
        printf("Error mapping page table level %d!: %i\n", 2, err);
        return;
    }
}