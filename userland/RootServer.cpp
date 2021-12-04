#include "RootServer.hpp"
#include "InitialUntypedPool.hpp"
#include "Syscall.hpp"
#include "kmalloc.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer()
    : _pt(_untypedPool),
      _factory(_untypedPool, _pt, ReservedVaddr + (ReservedPages * PAGE_SIZE)) {
  printf("Initialize Page Table\n");
  _pt.init(VirtualAddressLayout::AddressTables);
}

void RootServer::earlyInit() {
  printf("RootServer: reserve %zi pages\n", ReservedPages);
  reservePages();
  setMemoryPool((void *)VirtualAddressLayout::ReservedVaddr,
                ReservedPages * PAGE_SIZE);
}

void RootServer::lateInit() {
  auto apiEpOrErr = _factory.createEndpoint();
  assert(apiEpOrErr);
  _apiEndpoint = apiEpOrErr.value;

  printf("Test getting COM1\n");
  auto com1SlotOrErr = _untypedPool.getFreeSlot();
  assert(com1SlotOrErr);
  seL4_Error err = seL4_X86_IOPortControl_Issue(
      seL4_CapIOPortControl, 0x3F8, 0x3F8 + 7, seL4_CapInitThreadCNode,
      com1SlotOrErr.value, seL4_WordBits);
  assert(err == seL4_NoError);

  auto com1IRQSlotOrErr = _untypedPool.getFreeSlot();
  assert(com1IRQSlotOrErr);
#if 0
  seL4_Word ioapic = 0;
  seL4_Word com1pin = 4; // com1
  seL4_Word vector = 4;
  err = seL4_IRQControl_GetIOAPIC(seL4_CapIRQControl, seL4_CapInitThreadCNode,
                                  com1IRQSlotOrErr.value, seL4_WordBits, ioapic,
                                  com1pin, 0, 1, vector);
  assert(err == seL4_NoError);

  auto irqNotifOrErr = _factory.createNotification();
  assert(irqNotifOrErr);
  err = seL4_IRQHandler_SetNotification(com1IRQSlotOrErr.value,
                                        irqNotifOrErr.value);
  assert(err == seL4_NoError);
  _com1.irq = irqNotifOrErr.value;
#endif
  _com1port = com1SlotOrErr.value;
  _shell.init();
}

void RootServer::run() {
#if 0
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
#endif

  auto _comThOrErr = _factory.createThread(
      10,
      [this](Thread &t, void *) {
        _shell.start(t.endpoint);
        while (1) {
          seL4_X86_IOPort_In8_t d = seL4_X86_IOPort_In8(_com1port, 0x3F8);
          if (d.result) {
            _shell.onChar((char)d.result);
          }
        }
        return nullptr;
      },
      _apiEndpoint);
  if (_comThOrErr) {
    _comThOrErr.value.resume();
  }
  while (1) {
    seL4_Word sender = 0;
    seL4_MessageInfo_t msgInfo = seL4_Recv(_apiEndpoint, &sender);
    printf("RootTask: Received msg from %i %X\n", sender, seL4_GetMR(0));
    assert(seL4_MessageInfo_get_length(msgInfo) > 0);
    switch ((Syscall::Id)seL4_GetMR(0)) {
    case Syscall::Id::VMStats:
      printf("VMStats\n");
      printf("Num mapped pages %zi\n", _pt.getMappedPagesCount());
      printf("kmalloc'ed %zi/%zi bytes\n", getTotalKMallocated(),
             ReservedPages * PAGE_SIZE);
      break;
    default:
      break;
    }
    // seL4_Reply(msgInfo);
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