#pragma once
#include "InitialUntypedPool.hpp"
#include "lib/expected.hpp"
#include <cstddef>

struct Page {
  seL4_CPtr frame;
};

class PageTable {
public:
  using PageCapOrError = Expected<seL4_CPtr, seL4_Error>;

  PageTable(InitialUntypedPool &untypedPool) : untypedPool(untypedPool) {}

  void init(seL4_Word vaddr);

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