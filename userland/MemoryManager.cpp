#include "MemoryManager.hpp"
#include "runtime.h"
#include "sel4.hpp"



seL4_Error MemoryManager::mapPage(seL4_Word vaddr, seL4_CapRights_t rights)
{
    seL4_CPtr frame = InitialUntypedPool::instance().allocObject(seL4_X86_4K);
    return seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights, seL4_X86_Default_VMAttributes);
}

void MemoryManager::init()
{
    printf("MemoryManager: init\n");
    auto memPool = InitialUntypedPool::instance();
    pdpt = memPool.allocObject(seL4_X86_PDPTObject);
    pd = memPool.allocObject(seL4_X86_PageDirectoryObject);
    pt = memPool.allocObject(seL4_X86_PageTableObject);

    printf("pdpt is at %x\n", pdpt);
    printf("pd is at %x\n", pd);
    printf("pt is at %x\n", pt);

    seL4_Word vaddr = VirtualAddressLayout::AddressTables;

    /* map a PDPT at TEST_VADDR */
    seL4_Error error = seL4_X86_PDPT_Map(pdpt, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);

    error = seL4_X86_PageDirectory_Map(pd, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    assert(error == seL4_NoError, "seL4_X86_PageDirectory_Map");

    error = seL4_X86_PageTable_Map(pt, seL4_CapInitThreadVSpace, vaddr, seL4_X86_Default_VMAttributes);
    assert(error == seL4_NoError, "seL4_X86_PageTable_Map");

}


