#include "PageTable.hpp"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel
#include "runtime.h"
#include "sel4.hpp"

seL4_Error PageTable::mapPage(seL4_Word vaddr, seL4_CapRights_t rights)
{
    auto getLevel = [](seL4_Word lookupLevel) -> int{
        switch (lookupLevel)
        {
            case SEL4_MAPPING_LOOKUP_NO_PT:
                return 0;
            case SEL4_MAPPING_LOOKUP_NO_PD:
                return 1;
            case SEL4_MAPPING_LOOKUP_NO_PDPT:
                return 2;
            default:
                assert(false, "Unknown failed lookup level!");
                break;
        }
        NOT_REACHED();
        return -1;
    };
    seL4_CPtr frame = InitialUntypedPool::instance().allocObject(seL4_X86_4K);
    auto error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights, seL4_X86_Default_VMAttributes);
    if (error == seL4_FailedLookup)
    {
        int level = getLevel(seL4_MappingFailedLookupLevel());
        printf("Paging level error %i\n", level);
        if(level == 0)
        {
            auto memPool = InitialUntypedPool::instance();
            auto newPageTable = memPool.allocObject(seL4_X86_PageTableObject);
            printf("Alloc'ed a new page table cap\n");
            error = seL4_X86_PageTable_Map(newPageTable, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
            printf("Mapped a new page table err =%i\n", error);
            assert(error == seL4_NoError, "Handle error here :)");
            printf("Try the page mapping again:\n");
            error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights, seL4_X86_Default_VMAttributes);
            printf("Mapped a new page err =%i\n", error);
            return error;
        }
        else
        {
            assert(false, "Implement me :)");
        }
    }

    return error;
}

seL4_Error PageTable::unmapPage()
{
    seL4_CPtr frame = 0;
    return seL4_X86_Page_Unmap(frame);
}

void PageTable::init(seL4_Word vaddr)
{
    printf("PageTable: init\n");
    auto memPool = InitialUntypedPool::instance();
    pdpt = memPool.allocObject(seL4_X86_PDPTObject);
    pd = memPool.allocObject(seL4_X86_PageDirectoryObject);
    pt = memPool.allocObject(seL4_X86_PageTableObject);

    printf("pdpt is at %x\n", pdpt);
    printf("pd is at %x\n", pd);
    printf("pt is at %x\n", pt);

    /* map a PDPT at TEST_VADDR */
    seL4_Error error = seL4_X86_PDPT_Map(pdpt, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);

    error = seL4_X86_PageDirectory_Map(pd, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    assert(error == seL4_NoError, "seL4_X86_PageDirectory_Map");

    error = seL4_X86_PageTable_Map(pt, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    assert(error == seL4_NoError, "seL4_X86_PageTable_Map");
}


PageTable::PageCapOrError PageTable::test()
{
    return success<seL4_CPtr, seL4_Error>(1);
}