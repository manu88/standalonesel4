#include "RootServer.hpp"
#include "InitialUntypedPool.hpp"
#include "Process.hpp"
#include "Syscall.hpp"
#include "klog.h"
#include "kmalloc.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer()
    : _pt(_untypedPool), _vmspace(VMSpace::RootServerLayout::ReservedVaddr +
                                  (KmallocReservedPages * PAGE_SIZE)),
      _factory(_untypedPool, _pt, _vmspace) {
  kprintf("Initialize Page Table\n");
  _pt.init(VMSpace::RootServerLayout::AddressTables);
}

void RootServer::earlyInit() {
  seL4_SetUserData((seL4_Word)&Thread::main);

  assert(Thread::calledFromMain());
  kprintf("RootServer: reserve %zi pages\n", KmallocReservedPages);
  reservePages();
  auto kmallocNotif = _factory.createNotification();
  setMemoryPool((void *)VMSpace::RootServerLayout::ReservedVaddr,
                KmallocReservedPages * PAGE_SIZE, kmallocNotif.value);
}

void RootServer::lateInit() {
  auto apiEpOrErr = _factory.createEndpoint();
  assert(apiEpOrErr);
  _apiEndpoint = apiEpOrErr.value;

  _vmspace.delegate = this;
  Thread::main.vmspace = &_vmspace;

  assert(_platExpert.init(&_factory, &_pt));
  _vfs.init();

  for (auto dev : _platExpert.getBlockDevices()) {
    _vfs.inpectDev(*dev);
  }
  kprintf("Got %zi mountable partition(s)\n", _vfs.getMountables().size());
  if (_vfs.getMountables().size() > 0) {
    _vfs.mount(&_vfs.getMountables()[0], "/");
  }
  kprintf("Test getting COM1\n");
  auto com1SlotOrErr = _platExpert.issuePortRangeWithSize(0x3F8, 8);
  assert(com1SlotOrErr);

  _com1port = com1SlotOrErr.value;
  _shell.init();
  exec();
}

Expected<std::shared_ptr<Thread>, seL4_Error>
RootServer::createThread(Thread::EntryPoint entryPoint) {
  auto ret =
      _factory.createThread(_tcbBadgeCounter++, entryPoint, _apiEndpoint);
  if (ret) {
    _threads.add(ret.value);
    kprintf("Created new thread with badge %X\n", ret.value->badge);
  }

  return ret;
}

void RootServer::onTimerTick() {
  // called from timer thread!!
  ellapsedTime++;
  if (ellapsedTime % 100 == 0) {
    //    kprintf("One sec %zu\n", ellapsedTime);
  }
}

void RootServer::exec() {
  kprintf("Test process\n");
  return;
#if 0
  kprintf("Create ASID Pool\n");
  auto asidPoolOrErr = _untypedPool.getFreeSlot();//allocObject(seL4_X86_4K);
  assert(asidPoolOrErr);
  auto asidCapOrErr = _untypedPool.getFreeSlot();
  assert(asidCapOrErr);
  auto err = seL4_X86_ASIDControl_MakePool(seL4_CapASIDControl, asidPoolOrErr.value, seL4_CapInitThreadCNode, asidCapOrErr.value, seL4_WordBits);
  if(err != seL4_NoError){
    kprintf("seL4_X86_ASIDControl_MakePool Err %s\n", seL4::errorStr(err));
    assert(0);
  }
#endif
  kprintf("Create cspace\n");
  seL4_Error err = seL4_NoError;
  auto cspaceOrErr = _untypedPool.allocObject(seL4_CapTableObject);
  assert(cspaceOrErr);
  Process p;
  p.cspace = cspaceOrErr.value;

  auto pageDirOrErr =
      _untypedPool.allocObject(seL4_X86_PageDirectoryObject, cspaceOrErr.value);
  if (!pageDirOrErr) {
    kprintf("seL4_X86_PageDirectoryObject Err %s\n",
            seL4::errorStr(pageDirOrErr.error));
  }
  assert(pageDirOrErr);
  p.pageDir = pageDirOrErr.value;
  kprintf("Page Dir ok\n");
  err = seL4_X86_PageDirectory_Map(p.pageDir, 1, 0x1000000,
                                   seL4_X86_Default_VMAttributes);
  if (err != seL4_NoError) {
    kprintf("seL4_X86_PageDirectory_Map Err %s\n", seL4::errorStr(err));
    assert(0);
  }
  err = seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool, p.pageDir);
  if (err != seL4_NoError) {
    kprintf("seL4_X86_ASIDPool_Assign Err %s\n", seL4::errorStr(err));
    assert(0);
  }
}

void RootServer::run() {
#ifdef ARCH_X86_64
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
    _comThOrErr.value->vmspace = &_vmspace;
  }
#endif
  auto timeThread = createThread([this](Thread &, void *) {
    auto timerIRQ = _platExpert.getPitIRQ();
    while (true) {
      seL4_Wait(timerIRQ.notif, nullptr);
      onTimerTick();
      timerIRQ.ack();
    }
    return nullptr;
  });
  if (timeThread) {
    timeThread.value->setName("Timer");
    timeThread.value->start();
    timeThread.value->vmspace = &_vmspace;
  }

  auto testThread = createThread([this](Thread &, void *) {
    kprintf("TEST THREAD STARTED\n");
    while (1) {
      seL4_Yield();
    }
    return nullptr;
  });
  if (testThread) {
    testThread.value->setName("Test thread");
    testThread.value->start();
    testThread.value->vmspace = &_vmspace;
  }
  kprintf("Start rootServer runloop\n");
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
      kprintf("Cap fault to handle\n");
      break;
    case seL4_Fault_UnknownSyscall:
      kprintf("Unknown syscall to handle\n");
      break;
    case seL4_Fault_UserException:
      kprintf("User exception to handle\n");
      break;
    case seL4_Fault_VMFault:
      handleVMFault(msgInfo, *caller);
      break;
    default:
      break;
    }
  }
}

seL4_Error RootServer::mapPage(seL4_Word vaddr, seL4_CapRights_t rights,
                               seL4_Word &cap) {
  auto pageCapOrErr = _pt.mapPage(vaddr, rights);
  if (!pageCapOrErr) {
    return pageCapOrErr.error;
  }
  cap = pageCapOrErr.value;
  return seL4_NoError;
}

void RootServer::handleVMFault(const seL4_MessageInfo_t &msgInfo,
                               Thread &caller) {
  const seL4_Word programCounter = seL4_GetMR(seL4_VMFault_IP);
  const seL4_Word faultAddr = seL4_GetMR(seL4_VMFault_Addr);
  const seL4_Word isPrefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
  const seL4_Word faultStatusRegister = seL4_GetMR(seL4_VMFault_FSR);

  typedef struct {
    uint8_t present : 1; // P: When set, the page fault was caused by a
                         // page-protection violation. When not set, it was
                         // caused by a non-present page.
    uint8_t write : 1;   // W: When set, the page fault was caused by a write
                       // access. When not set, it was caused by a read access.
    uint8_t user : 1; // U: When set, the page fault was caused while CPL = 3.
                      // This does not necessarily mean that the page fault was
                      // a privilege violation.
    uint8_t reservedWrite : 1;    // R: When set, one or more page directory
                                  // entries contain reserved bits which are set
                                  // to 1. This only applies when the PSE or PAE
                                  // flags in CR4 are set to 1.
    uint8_t instructionFetch : 1; // I: When set, one or more page directory
                                  // entries contain reserved bits which are set
                                  // to 1. This only applies when the PSE or PAE
                                  // flags in CR4 are set to 1.
  } PageFault; // see https://wiki.osdev.org/Exceptions#Page_Fault

  PageFault *fault = (PageFault *)&faultStatusRegister;

  auto faultyVmspace = caller.vmspace;
  assert(faultyVmspace != nullptr);

  auto resSlot = faultyVmspace->getReservationForAddress(faultAddr);
  if (resSlot.first >= 0) {
    bool ret = faultyVmspace->mapPage(faultAddr);
    if (resSlot.second.type == VMSpace::MemoryType::IPC) {
      caller.setIPCBuffer(resSlot.second.vaddr, resSlot.second.pageCap);
      kprintf("Set IPC buffer for thread %X\n", caller.badge);
    }
    if (ret) {
      seL4_Reply(msgInfo);
    }
  } else {
    faultyVmspace->print();
    kprintf("Fault P=%u W=%u U=%u R=%u I=%u\n", fault->present, fault->write,
            fault->user, fault->reservedWrite, fault->instructionFetch);
    kprintf("programCounter      0X%lX\n", programCounter);
    kprintf("faultAddr           0X%lX\n", faultAddr);
    kprintf("isPrefetch          0X%lX\n", isPrefetch);
    kprintf("faultStatusRegister 0X%lX\n", faultStatusRegister);
    kprintf("faulty page was NOT reserved\n");
    kprintf("TODO: terminate caller\n");
  }
}

void RootServer::processSyscall(const seL4_MessageInfo_t &msgInfo,
                                Thread &caller) {
  assert(seL4_MessageInfo_get_length(msgInfo) > 0);
  seL4_Word syscallID = seL4_GetMR(0);
  switch ((Syscall::ID)syscallID) {
  case Syscall::ID::Open: {
    auto msgOrErr = Syscall::OpenRequest::decode(msgInfo);
    if (msgOrErr) {
      auto fileOrErr = _vfs.open(msgOrErr.value.inodeId);
      if (!fileOrErr) {
        seL4_SetMR(1, 1);
        seL4_Reply(msgInfo);
        break;
      }
      testFile = *fileOrErr;
      kprintf("File open ok pos=%zi inode=%p size=%zu\n", testFile.pos,
              testFile.inode, testFile.getSize());
    }
    seL4_SetMR(1, 0);
    seL4_Reply(msgInfo);

  } break;
  case Syscall::ID::Read: {
    auto paramOrErr = Syscall::ReadRequest::decode(msgInfo);
    if (!paramOrErr) {
      seL4_SetMR(1, -1);
      seL4_Reply(msgInfo);
    }
    kprintf("Read request inode %zi arg %zi\n", paramOrErr.value.inodeId,
            paramOrErr.value.size);
    if (paramOrErr.value.inodeId == 2) {
      _vfs.testRead();
      seL4_SetMR(1, 0);
      seL4_Reply(msgInfo);
    } else {
      if (paramOrErr.value.size == 0) {
        seL4_SetMR(1, -2);
        seL4_Reply(msgInfo);
        return;
      }
      auto data = reinterpret_cast<uint8_t *>(kmalloc(paramOrErr.value.size));
      if (!data) {
        seL4_SetMR(1, -3);
        seL4_Reply(msgInfo);
        return;
      }
      auto ret = _vfs.read(testFile, data, paramOrErr.value.size);
      kprintf("read returned %zi, new pos is %zu\n", ret, testFile.pos);
      if (ret > 0 && paramOrErr.value.size) {
        kprintf("'%s'", (const char *)data);
      }
      kfree(data);
      seL4_SetMR(1, ret);
      seL4_Reply(msgInfo);
    }
  } break;
  case Syscall::ID::Sleep: {
    auto paramOrErr = Syscall::SleepRequest::decode(msgInfo);
    if (paramOrErr) {
      kprintf("Request to sleep %i ms\n", paramOrErr.value.ms);
    }
  } break;
  case Syscall::ID::Debug: {
    auto paramOrErr = Syscall::DebugRequest::decode(msgInfo);
    if (paramOrErr) {
      if (paramOrErr.value.op == Syscall::DebugRequest::Operation::VMStats) {
        kprintf("VMStats\n");
        kprintf("Num mapped pages %zi\n", _pt.getMappedPagesCount());
        kprintf("kmalloc'ed %zi/%zi bytes\n", getTotalKMallocated(),
                KmallocReservedPages * PAGE_SIZE);
        kmallocPrintStats();
        _vmspace.print();
        _untypedPool.print();
      } else if (paramOrErr.value.op ==
                 Syscall::DebugRequest::Operation::DumpScheduler) {
        seL4_DebugDumpScheduler();
        for (const auto &t : _threads.threads) {
          kprintf("Thread: badge %X endpoint %X prio %i %s\n", t->badge,
                  t->endpoint, t->priority,
                  (caller == *t ? "<- Calling thread" : ""));
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
      kprintf("kfreeing %lu\n", paramOrErr.value.ptr);
      kfree(paramOrErr.value.ptr);
      seL4_SetMR(1, 0);
      seL4_Reply(msgInfo);
    }
  } break;
  case Syscall::ID::Thread: {
    auto paramOrErr = Syscall::ThreadRequest::decode(msgInfo);
    if (paramOrErr) {
      switch (paramOrErr.value.op) {
      case Syscall::ThreadRequest::List:
        seL4_DebugDumpScheduler();
        for (const auto &t : _threads.threads) {
          kprintf("Thread: badge %X endpoint %X prio %i status %s %s\n",
                  t->badge, t->endpoint, t->priority,
                  Thread::getStateStr(t->getState()),
                  (caller == *t ? "<- Calling thread" : ""));
        }
        break;
      case Syscall::ThreadRequest::Suspend: {
        kprintf("Request to suspend %X\n", paramOrErr.value.arg1);
        auto threadToSuspend = _threads.get(paramOrErr.value.arg1);
        if (threadToSuspend) {
          threadToSuspend->suspend();
        } else {
          kprintf("Thread not found\n");
        }
      } break;
      case Syscall::ThreadRequest::Resume: {
        kprintf("Request to resume %X\n", paramOrErr.value.arg1);
        auto threadToResume = _threads.get(paramOrErr.value.arg1);
        if (threadToResume) {
          threadToResume->resume();
        } else {
          kprintf("Thread not found\n");
        }
      } break;
      case Syscall::ThreadRequest::SetPriority: {
        kprintf("Change priority %X\n", paramOrErr.value.arg1);
        auto threadToResume = _threads.get(paramOrErr.value.arg1);
        if (threadToResume) {
          threadToResume->setPriority(paramOrErr.value.arg2);
        } else {
          kprintf("Thread not found\n");
        }
      } break;
      case Syscall::ThreadRequest::VM: {
        auto thread = _threads.get(paramOrErr.value.arg1);
        if (thread) {
          thread->vmspace->print();
        } else {
          kprintf("Thread not found\n");
        }
      } break;
      case Syscall::ThreadRequest::StopAndDelete: {
        kprintf("Stop and delete %X\n", paramOrErr.value.arg1);
        auto thread = _threads.get(paramOrErr.value.arg1);
        if (thread) {
          thread->suspend();
        } else {
          kprintf("Thread not found\n");
        }

      } break;
      }
      seL4_SetMR(1, 0);
      seL4_Reply(msgInfo);
    }
    break;
  }
  case Syscall::ID::MMap: {
    auto paramOrErr = Syscall::MMapRequest::decode(msgInfo);
    if (paramOrErr) {
      kprintf("mmap request for %zi pages\n", paramOrErr.value.numPages);
      auto res = _vmspace.allocRangeAnywhere(paramOrErr.value.numPages);
      _vmspace.print();
      if (res) {
        seL4_SetMR(1, res.value.vaddr);
      } else {
        seL4_SetMR(1, 0);
      }
      seL4_Reply(msgInfo);
    }
  } break;
  case Syscall::ID::Platform: {
    _platExpert.print();
  } break;
  case Syscall::ID::Poweroff: {
    _platExpert.doPowerOff();
  } break;
  default:
    kprintf("RootTask: Received msg %X from badge %i\n", syscallID,
            caller.badge);
    break;
  }
}

void RootServer::reservePages() {
  seL4_Word vaddr = VMSpace::RootServerLayout::ReservedVaddr;
  for (int i = 0; i < KmallocReservedPages; i++) {
    auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
    assert(capOrError);
    vaddr += PAGE_SIZE;
  }
  auto endVaddr = vaddr;
  vaddr = VMSpace::RootServerLayout::ReservedVaddr;
  kprintf("Test reserved pages between %X and %X\n", vaddr, endVaddr);
  for (int i = 0; i < KmallocReservedPages; i++) {
    memset((void *)vaddr, 0, PAGE_SIZE);
    vaddr += PAGE_SIZE;
  }
  vaddr = VMSpace::RootServerLayout::ReservedVaddr;
  for (int i = 0; i < KmallocReservedPages; i++) {
    for (int j = 0; j < PAGE_SIZE; j++) {
      assert(reinterpret_cast<char *>(vaddr)[j] == 0);
      (reinterpret_cast<char *>(vaddr))[j] = 0;
    }
  }
}

void RootServer::testPt() {
  seL4_Word vaddr = VMSpace::RootServerLayout::ReservedVaddr +
                    (KmallocReservedPages * PAGE_SIZE);

  size_t sizeToTest = 1024;
  for (size_t i = 0; i < sizeToTest; i++) {
    auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
    if (capOrError.error == seL4_FailedLookup) {
      kprintf("Missing intermediate paging structure at level %lu\n",
              seL4_MappingFailedLookupLevel());
    } else if (capOrError.error != seL4_NoError) {
      kprintf("Test mapping error = %i at %i\n", capOrError.error, i);
    }
    auto test = reinterpret_cast<size_t *>(vaddr);
    *test = i;
    assert(*test == i);
    vaddr += 4096;
    if (i % 100 == 0) {
      kprintf("Page %zi/%zi ok\n", i, sizeToTest);
    }
  }

  kprintf("After test, vaddr is at %X\n", vaddr);
  vaddr = VMSpace::RootServerLayout::ReservedVaddr +
          (KmallocReservedPages * PAGE_SIZE);
  for (size_t i = 0; i < sizeToTest; i++) {
    auto test = reinterpret_cast<size_t *>(vaddr);
    //        *test = i;
    assert(*test == i);
    vaddr += 4096;
  }
  kprintf("After test OK \n");
}