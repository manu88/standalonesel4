#include "RootServer.hpp"
#include "InitialUntypedPool.hpp"
#include "kmalloc.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer()
    : _pt(_untypedPool),
      _factory(_untypedPool, _pt, ReservedVaddr + (ReservedPages * PAGE_SIZE)) {
  printf("Initialize Page Table\n");
  _pt.init(VirtualAddressLayout::AddressTables);
}

void RootServer::run() {
  printf("RootServer: reserve %zi pages\n", ReservedPages);
  reservePages();
  setMemoryPool((void *)VirtualAddressLayout::ReservedVaddr,
                ReservedPages * PAGE_SIZE);

  void *dat = kmalloc(1024);
  assert(dat != nullptr);
  kfree(dat);

  auto apiEpOrErr = _factory.createEndpoint();
  assert(apiEpOrErr);
  _apiEndpoint = apiEpOrErr.value;

  Thread threads[10] = {};
  for (int i = 0; i < 10; i++) {
    auto threadOrErr = _factory.createThread(
        i,
        [](Thread &t, void *) {
          printf("Hello from THREAD %X\n", t.badge);
          auto info = seL4_MessageInfo_new(0, 0, 0, 0);
          seL4_Call(t.endpoint, info);
          printf("THREAD %X: call returned\n", t.badge);

          return nullptr;
        },
        _apiEndpoint);
    if (threadOrErr) {
      threads[i] = threadOrErr.value;
      threads[i].resume();
    }
  }

  while (1) {
    seL4_Word sender = 0;
    seL4_MessageInfo_t msgInfo = seL4_Recv(_apiEndpoint, &sender);
    printf("RootTask: Received msg from %i\n", sender);
    seL4_Reply(msgInfo);
  }
}

void RootServer::reservePages() {
  seL4_Word vaddr = VirtualAddressLayout::ReservedVaddr;
  for (int i = 0; i < ReservedPages; i++) {
    auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
    assert(capOrError);
    vaddr += PAGE_SIZE;
  }

  printf("Test reserved pages\n");
  vaddr = VirtualAddressLayout::ReservedVaddr;
  for (int i = 0; i < ReservedPages; i++) {
    memset((void *)vaddr, 0, PAGE_SIZE);
    vaddr += PAGE_SIZE;
  }
  vaddr = VirtualAddressLayout::ReservedVaddr;
  for (int i = 0; i < ReservedPages; i++) {
    for (int j = 0; j < PAGE_SIZE; j++) {
      assert(reinterpret_cast<char *>(vaddr)[j] == 0);
      (reinterpret_cast<char *>(vaddr))[j] = 0;
    }
  }
}

void RootServer::testPt() {
  seL4_Word vaddr =
      VirtualAddressLayout::ReservedVaddr + (ReservedPages * PAGE_SIZE);

  size_t sizeToTest = 1024;
  for (size_t i = 0; i < sizeToTest; i++) {
    auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
    if (capOrError.error == seL4_FailedLookup) {
      printf("Missing intermediate paging structure at level %lu\n",
             seL4_MappingFailedLookupLevel());
    } else if (capOrError.error != seL4_NoError) {
      printf("Test mapping error = %i at %i\n", capOrError.error, i);
    }
    auto test = reinterpret_cast<size_t *>(vaddr);
    *test = i;
    assert(*test == i);
    vaddr += 4096;
    if (i % 100 == 0) {
      printf("Page %zi/%zi ok\n", i, sizeToTest);
    }
  }

  printf("After test, vaddr is at %X\n", vaddr);
  vaddr = VirtualAddressLayout::ReservedVaddr + (ReservedPages * PAGE_SIZE);
  for (size_t i = 0; i < sizeToTest; i++) {
    auto test = reinterpret_cast<size_t *>(vaddr);
    //        *test = i;
    assert(*test == i);
    vaddr += 4096;
  }
  printf("After test OK \n");
}