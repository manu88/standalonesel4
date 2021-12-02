#pragma once
#include "Thread.hpp"
#include "lib/expected.hpp"

class PageTable;
class InitialUntypedPool;
class ObjectFactory {
public:
  using ObjectOrError = Expected<seL4_CPtr, seL4_Error>;
  ObjectFactory(InitialUntypedPool &, PageTable &,
                seL4_Word currentVirtualAddress);
  Expected<Thread, seL4_Error> createThread(seL4_Word tcbBadge,
                                            Thread::EntryPoint entryPoint,
                                            seL4_CPtr apiEndpoint);

  ObjectOrError createEndpoint();
  seL4_Word currentVirtualAddress = 0;

private:
  InitialUntypedPool &_untypedPool;
  PageTable &_pt;
};