#pragma once
#include "InitialUntypedPool.hpp"
#include "ObjectFactory.hpp"
#include "PageTable.hpp"
#include "Thread.hpp"
#include "lib/expected.hpp"

class RootServer {
public:
  RootServer();
  void init(); // kmalloc/kfree/new/delete are setup here!
  void run();

private:
  enum { ReservedPages = 10 };
  enum VirtualAddressLayout // Layout of root server, not other processes!!
  { AddressTables = 0x8000000000,
    ReservedVaddr = 0x8000001000, // size is ReservedPages pages
  };

  void reservePages();
  void testPt();

  InitialUntypedPool _untypedPool;
  PageTable _pt;
  ObjectFactory _factory;
  seL4_CPtr _apiEndpoint = 0;
};