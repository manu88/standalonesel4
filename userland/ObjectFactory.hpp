#pragma once
#include "Thread.hpp"
#include "lib/expected.hpp"
#include <memory>

class PageTable;
class InitialUntypedPool;
class VMSpace;
class Process;

class ObjectFactory {
public:
  using ObjectOrError = Expected<seL4_CPtr, seL4_Error>;
  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;

  ObjectFactory(InitialUntypedPool &, PageTable &, VMSpace &);

  Expected<std::shared_ptr<Thread>, seL4_Error>
  createThread(seL4_Word tcbBadge, Thread::EntryPoint entryPoint,
               seL4_CPtr apiEndpoint);
  
  seL4_Error configProcess(Process &p, seL4_CPtr apiEndpoint);
  ObjectOrError createEndpoint();
  ObjectOrError createNotification();
  void releaseObject(seL4_CPtr);

  SlotOrError getFreeSlot();
  void releaseSlot(seL4_SlotPos pos);

private:
  InitialUntypedPool &_untypedPool;
  PageTable &_pt;
  VMSpace &_vmSpace;
};