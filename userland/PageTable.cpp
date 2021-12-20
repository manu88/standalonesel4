#include "PageTable.hpp"
#include "runtime.h"
#include "sel4.hpp"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

//#ifdef ARCH_X86_64
PageTable::PageCapOrError PageTable::mapPage(seL4_Word vaddr,
                                             seL4_CapRights_t rights) {
  auto getLevel = [](seL4_Word lookupLevel) -> int {
    switch (lookupLevel) {
    case SEL4_MAPPING_LOOKUP_NO_PT:
      return 0;
    case SEL4_MAPPING_LOOKUP_NO_PD:
      return 1;
    case SEL4_MAPPING_LOOKUP_NO_PDPT:
      return 2;
    default:
      kprintf("Unknown failed lookup level %i!", lookupLevel);
      assert(false);
      break;
    }
    NOT_REACHED();
    return -1;
  };

  auto frameOrErr = untypedPool.allocObject(seL4_X86_4K);
  assert(frameOrErr);
  seL4_CPtr frame = frameOrErr.value;
  auto error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights,
                                 seL4_X86_Default_VMAttributes);
  if (error == seL4_FailedLookup) {
    int level = getLevel(seL4_MappingFailedLookupLevel());
    kprintf("Paging level error %i\n", level);
    if (level == 0) {
      auto newPageTable = untypedPool.allocObject(seL4_X86_PageTableObject);
      kprintf("Alloc'ed a new page table cap\n");
      error =
          seL4_X86_PageTable_Map(newPageTable.value, seL4_CapInitThreadVSpace,
                                 vaddr, seL4_X86_Default_VMAttributes);
      kprintf("Mapped a new page table err =%i\n", error);
      assert(error == seL4_NoError);
      kprintf("Try the page mapping again:\n");
      error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights,
                                seL4_X86_Default_VMAttributes);
      kprintf("Mapped a new page err =%i\n", error);
      _mappedPages++;
      return success<seL4_CPtr, seL4_Error>(frame);
    } else {
      kprintf("Implement me :)\n");
      assert(false);
    }
  }
  _mappedPages++;
  return success<seL4_CPtr, seL4_Error>(frame);
}

seL4_Error PageTable::unmapPage(seL4_CPtr pageCap) {
  seL4_Error err = seL4_X86_Page_Unmap(pageCap);
  if (err == seL4_NoError) {
    _mappedPages--;
  }
  return err;
}

void PageTable::initIOPages(seL4_Word vaddr){
  kprintf("PageTable: initIOPages\n");
#define FAKE_PCI_DEVICE 0x216u
#define DOMAIN_ID       0x0
  auto iospace = untypedPool.getFreeSlot();
  assert(iospace);
  auto err = seL4_CNode_Mint(seL4_CapInitThreadCNode, iospace.value, seL4_WordBits, 
                  seL4_CapIOSpace, seL4_CapIOSpace, seL4_WordBits,
                  seL4_AllRights, (DOMAIN_ID << 16) | FAKE_PCI_DEVICE
                  );
  kprintf("seL4_CNode_Mint err=%s\n", seL4::errorStr(err));
#if 0
  auto ioFrameOrErr = untypedPool.allocObject(seL4_X86_4K);
  if(!ioFrameOrErr){
    kprintf("allocObject(seL4_X86_IOPageTableObject) err %s\n" ,seL4::errorStr(ioFrameOrErr.error));
  }
  assert(ioFrameOrErr);

  int i = 0;
  while (i<3){ //seL4_X86_Page_MapIO(ioFrameOrErr.value, seL4_CapIOSpace, seL4_AllRights, vaddr) == seL4_FailedLookup){
    kprintf("Alloc intermediate page iter %i\n", i);
    auto ioPageOrErr = untypedPool.allocObject(seL4_X86_IOPageTableObject);
    assert(ioPageOrErr);
    seL4_X86_IOSpace iospace = untypedPool.getFreeSlot();
    auto err = seL4_X86_IOPageTable_Map(ioPageOrErr.value, iospace, vaddr);
    if(err != seL4_NoError){
      kprintf("seL4_X86_IOPageTable_Map err %s\n" ,seL4::errorStr(err));
    }
    assert(err == seL4_NoError);
    i++;
  }
  kprintf("End while loop\n");
//  auto err = seL4_X86_Page_MapIO(ioFrameOrErr.value, seL4_CapIOSpace, seL4_AllRights, vaddr);
//  kprintf("seL4_X86_Page_MapIO err %s\n" ,seL4::errorStr(err));
#endif
}

void PageTable::init(seL4_Word vaddr) {
  kprintf("PageTable: init\n");
  auto pdptOrErr = untypedPool.allocObject(seL4_X86_PDPTObject);
  pdpt = pdptOrErr.value;
  auto pdOrErr = untypedPool.allocObject(seL4_X86_PageDirectoryObject);
  pd = pdOrErr.value;
  auto ptOrErr = untypedPool.allocObject(seL4_X86_PageTableObject);
  pt = ptOrErr.value;

  kprintf("pdpt is at %x\n", pdpt);
  kprintf("pd is at %x\n", pd);
  kprintf("pt is at %x\n", pt);

  /* map a PDPT at TEST_VADDR */
  seL4_Error error = seL4_X86_PDPT_Map(pdpt, seL4_CapInitThreadVSpace, vaddr,
                                       seL4_X86_Default_VMAttributes);

  error = seL4_X86_PageDirectory_Map(pd, seL4_CapInitThreadVSpace, vaddr,
                                     seL4_X86_Default_VMAttributes);
  assert(error == seL4_NoError);

  error = seL4_X86_PageTable_Map(pt, seL4_CapInitThreadVSpace, vaddr,
                                 seL4_X86_Default_VMAttributes);
  assert(error == seL4_NoError);
}

#if 0 //defined(ARCH_ARM)

void PageTable::init(seL4_Word vaddr) {}

PageTable::PageCapOrError PageTable::mapPage(seL4_Word vaddr,
                                             seL4_CapRights_t rights) {
  return unexpected<seL4_CPtr, seL4_Error>(seL4_NotEnoughMemory);
}

seL4_Error PageTable::unmapPage(seL4_CPtr pageCap) {
  return seL4_NotEnoughMemory;
}
#endif