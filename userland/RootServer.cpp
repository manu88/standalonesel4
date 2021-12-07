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
  seL4_SetUserData((seL4_Word)&Thread::main);

  assert(Thread::calledFromMain());
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

Expected<std::shared_ptr<Thread>, seL4_Error>
RootServer::createThread(Thread::EntryPoint entryPoint) {
  auto ret =
      _factory.createThread(_tcbBadgeCounter++, entryPoint, _apiEndpoint);
  if (ret) {
    _threads.add(ret.value);
    printf("Created new thread with badge %X\n", ret.value->badge);
  }

  return ret;
}

void RootServer::run() {
  auto _comThOrErr = createThread([this](Thread &, void *) {
    _shell.start();
    while (1) {
      seL4_X86_IOPort_In8_t d = seL4_X86_IOPort_In8(_com1port, 0x3F8);
      if (d.result) {
        _shell.onChar((char)d.result);
      }
    }
    return nullptr;
  });
  if (_comThOrErr) {
    _comThOrErr.value->setName("Com1");
    _comThOrErr.value->start();
  }

  auto testThread = createThread([this](Thread &, void *) {
    printf("TEST THREAD STARTED\n");
    while (1){
      seL4_Yield();
    }
    return nullptr;});
  if(testThread){
    testThread.value->setName("Test thread");
    testThread.value->start();
  }

  printf("Start rootServer runloop\n");
  while (1) {
    seL4_Word sender = 0;
    seL4_MessageInfo_t msgInfo = seL4_Recv(_apiEndpoint, &sender);
    auto caller = _threads.get(sender);
    assert(caller != nullptr);
    switch (seL4_MessageInfo_get_label(msgInfo)) {
    case seL4_Fault_NullFault:
      processSyscall(msgInfo, *caller);
      break;
    case seL4_Fault_CapFault:
      printf("Cap fault to handle\n");
      break;
    case seL4_Fault_UnknownSyscall:
      printf("Unknown syscall to handle\n");
      break;
    case seL4_Fault_UserException:
      printf("User exception to handle\n");
      break;
    case seL4_Fault_VMFault:
      printf("VM fault to handle\n");
      break;
    default:
      break;
    }
  }
}

void RootServer::processSyscall(const seL4_MessageInfo_t &msgInfo,
                                Thread &caller) {
  assert(seL4_MessageInfo_get_length(msgInfo) > 0);
  seL4_Word syscallID = seL4_GetMR(0);
  switch ((Syscall::ID)syscallID) {
  case Syscall::ID::Debug: {
    auto paramOrErr = Syscall::DebugRequest::decode(msgInfo);
    if (paramOrErr) {
      if (paramOrErr.value.op == Syscall::DebugRequest::Operation::VMStats) {
        printf("VMStats\n");
        printf("Num mapped pages %zi\n", _pt.getMappedPagesCount());
        printf("kmalloc'ed %zi/%zi bytes\n", getTotalKMallocated(),
               ReservedPages * PAGE_SIZE);
      } else if (paramOrErr.value.op ==
                 Syscall::DebugRequest::Operation::DumpScheduler) {
        seL4_DebugDumpScheduler();
        for (const auto &t : _threads.threads) {
          printf("Thread: badge %X endpoint %X prio %i %s\n", t->badge, t->endpoint,
                 t->priority, (caller == *t? "<- Calling thread": ""));
        }
      }
    }
  } break;
  case Syscall::ID::KMalloc: {
    auto paramOrErr = Syscall::KMallocRequest::decode(msgInfo);
    if (paramOrErr) {
      void *ret = kmalloc(paramOrErr.value.size);
      seL4_SetMR(1, (seL4_Word)ret);
      seL4_Reply(msgInfo);
    }
  } break;
  case Syscall::ID::KFree: {
    auto paramOrErr = Syscall::KFreeRequest::decode(msgInfo);
    if (paramOrErr) {
      printf("kfreeing %lu\n", paramOrErr.value.ptr);
      kfree(paramOrErr.value.ptr);
      seL4_SetMR(1, 0);
      seL4_Reply(msgInfo);
    }
  } break;
  case Syscall::ID::Thread:{
    auto paramOrErr = Syscall::ThreadRequest::decode(msgInfo);
    if (paramOrErr){
      switch(paramOrErr.value.op){
        case Syscall::ThreadRequest::List:
        seL4_DebugDumpScheduler();
        for (const auto &t : _threads.threads) {
          printf("Thread: badge %X endpoint %X prio %i status %i %s\n", t->badge, t->endpoint,
                 t->priority, t->getState(), (caller == *t? "<- Calling thread": ""));
        } break;
        case Syscall::ThreadRequest::Suspend:{
          printf("Request to suspend %X\n", paramOrErr.value.arg1);
          auto threadToSuspend = _threads.get(paramOrErr.value.arg1);
          if(threadToSuspend){
            threadToSuspend->suspend();
          }else{
            printf("Thread not found\n");
          }
        }break;
        case Syscall::ThreadRequest::Resume:{
          printf("Request to resume %X\n", paramOrErr.value.arg1);
          auto threadToResume = _threads.get(paramOrErr.value.arg1);
          if(threadToResume){
            threadToResume->resume();
          }else{
            printf("Thread not found\n");
          }
        }break;
        case Syscall::ThreadRequest::SetPriority:{
          printf("Change priority %X\n", paramOrErr.value.arg1);
          auto threadToResume = _threads.get(paramOrErr.value.arg1);
          if(threadToResume){
            threadToResume->setPriority(paramOrErr.value.arg2);
          }else{
            printf("Thread not found\n");
          }
        }break;
      }
      seL4_SetMR(1, 0);
      seL4_Reply(msgInfo);
    } break;
  }
  case Syscall::ID::MMap: {
    auto paramOrErr = Syscall::MMapRequest::decode(msgInfo);
    if (paramOrErr) {
      printf("mmap request for %zi pages\n", paramOrErr.value.numPages);
      seL4_SetMR(1, 12345);
      seL4_Reply(msgInfo);
    }
  } break;
  default:
    printf("RootTask: Received msg %X from badge %i\n", syscallID, caller.badge);
    break;
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