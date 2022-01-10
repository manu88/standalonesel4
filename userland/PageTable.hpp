#pragma once
#include "InitialUntypedPool.hpp"
#include "lib/expected.hpp"
#include "lib/vector.hpp"
#include <cstddef>

class PageTable {
public:
  using PageCapOrError = Expected<seL4_CPtr, seL4_Error>;

  PageTable(InitialUntypedPool &untypedPool) : untypedPool(untypedPool) {}

  void init(seL4_Word vaddr, seL4_X64_PML4 pml4);

  PageCapOrError mapPage(seL4_Word vaddr, seL4_CapRights_t rights);
  seL4_Error unmapPage(seL4_CPtr pageCap);

  size_t getMappedPagesCount() const noexcept { return _mappedPages; }

private:
  InitialUntypedPool &untypedPool;
  seL4_CPtr pdpt = 0;
  seL4_CPtr pd = 0;
  seL4_CPtr pt = 0;

  size_t _mappedPages = 0;
};