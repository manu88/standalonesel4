#include "PageTable.hpp"
#include "runtime.h"
#include "sel4.hpp"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

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
      printf("Unknown failed lookup level %i!", lookupLevel);
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
    printf("Paging level error %i\n", level);
    if (level == 0) {
      auto newPageTable = untypedPool.allocObject(seL4_X86_PageTableObject);
      printf("Alloc'ed a new page table cap\n");
      error = seL4_X86_PageTable_Map(newPageTable, seL4_CapInitThreadVSpace,
                                     vaddr, seL4_X86_Default_VMAttributes);
      printf("Mapped a new page table err =%i\n", error);
      assert(error == seL4_NoError);
      printf("Try the page mapping again:\n");
      error = seL4_X86_Page_Map(frame, seL4_CapInitThreadVSpace, vaddr, rights,
                                seL4_X86_Default_VMAttributes);
      printf("Mapped a new page err =%i\n", error);
      _mappedPages++;
      return success<seL4_CPtr, seL4_Error>(frame);
    } else {
      printf("Implement me :)\n");
      assert(false);
    }
  }
  return success<seL4_CPtr, seL4_Error>(frame);
}

seL4_Error PageTable::unmapPage(seL4_CPtr pageCap) {
  seL4_Error err = seL4_X86_Page_Unmap(pageCap);
  if (err == seL4_NoError) {
    _mappedPages--;
  }
  return err;
}

void PageTable::init(seL4_Word vaddr) {
  printf("PageTable: init\n");
  auto pdptOrErr = untypedPool.allocObject(seL4_X86_PDPTObject);
  pdpt = pdptOrErr.value;
  auto pdOrErr = untypedPool.allocObject(seL4_X86_PageDirectoryObject);
  pd = pdOrErr.value;
  auto ptOrErr = untypedPool.allocObject(seL4_X86_PageTableObject);
  pt = ptOrErr.value;

  printf("pdpt is at %x\n", pdpt);
  printf("pd is at %x\n", pd);
  printf("pt is at %x\n", pt);

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
