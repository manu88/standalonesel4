#pragma once
#include "InitialUntypedPool.hpp"
#include "ObjectFactory.hpp"
#include "PageTable.hpp"
#include "Shell.hpp"
#include "Thread.hpp"
#include "lib/expected.hpp"
#include "lib/vector.hpp"

struct ThreadTable {
  vector<Thread> threads;

  void add(const Thread &t) { threads.push_back(t); }
};

class RootServer {
public:
  RootServer();
  void earlyInit(); // kmalloc/kfree/new/delete are setup here!
  void lateInit();
  void run();
  void processSyscall(const seL4_MessageInfo_t &msgInfo, seL4_Word sender);

private:
  Expected<Thread, seL4_Error> createThread(Thread::EntryPoint entryPoint);
  enum { ReservedPages = 10 };
  enum VirtualAddressLayout // Layout of root server, not other processes!!
  { AddressTables = 0x8000000000,
    ReservedVaddr = 0x8000001000, // size is ReservedPages pages
  };

  void reservePages();
  void testPt();

  ThreadTable _threads;
  Shell _shell;
  seL4_CPtr _com1port;

  InitialUntypedPool _untypedPool;
  PageTable _pt;
  ObjectFactory _factory;
  seL4_CPtr _apiEndpoint = 0;
  seL4_Word _tcbBadgeCounter = 1;
};