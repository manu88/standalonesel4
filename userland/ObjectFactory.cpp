#include "ObjectFactory.hpp"
#include "InitialUntypedPool.hpp"
#include "PageTable.hpp"
#include "VMSpace.hpp"
#include "klog.h"
#include "runtime.h"
#include "Process.hpp"


ObjectFactory::ObjectFactory(InitialUntypedPool &untypedPool, PageTable &pt,
                             VMSpace &vm)
    : _untypedPool(untypedPool), _pt(pt), _vmSpace(vm) {}


seL4_Error ObjectFactory::configProcess(Process &process, seL4_CPtr /*apiEndpoint*/){
  kprintf("Start process configuration\n");

  auto tcbOrErr = _untypedPool.allocObject(seL4_TCBObject);
  if (!tcbOrErr) {
    return tcbOrErr.error;
  }
  process.tcb = tcbOrErr.value;
  seL4_DebugNameThread(process.tcb, "process");
  auto error = seL4_NoError;

  auto tcbTlsOrErr = process._vmspace.allocRangeAnywhere(1, seL4_ReadWrite);
  assert(tcbTlsOrErr);
  auto tcbIPCOrErr = _vmSpace.allocIPCBuffer(seL4_ReadWrite);
  assert(tcbIPCOrErr);

  error = seL4_TCB_SetTLSBase(process.tcb, tcbTlsOrErr.value.vaddr);
  if(error != seL4_NoError){
    kprintf("seL4_TCB_SetTLSBase error %s\n", seL4::errorStr(error));
  }
  seL4_Word prio = seL4_MaxPrio;
  
  error = seL4_TCB_SetPriority(process.tcb, seL4_CapInitThreadTCB, prio);
    if(error != seL4_NoError){
    kprintf("seL4_TCB_SetPriority error %s\n", seL4::errorStr(error));
  }
#if 0
  kprintf("Start Test mint\n");
  error = seL4_CNode_Mint(process.cspace, Process::CSpaceSlot(Process::CSpaceLayout::ENDPOINT_SLOT), seL4_WordBits,
                        seL4_CapInitThreadCNode, apiEndpoint, seL4_WordBits,
                        seL4_AllRights, (seL4_Word) &process);
  if(error != seL4_NoError){
    kprintf("seL4_CNode_Mint endpoint error %s\n", seL4::errorStr(error));
  }
  kprintf("End Test mint\n");
#endif
  error = seL4_TCB_SetSpace(process.tcb, 0, process.cspace, 0, process.vspace, 0);
  if(error != seL4_NoError){
    kprintf("seL4_TCB_SetSpace endpoint error %s\n", seL4::errorStr(error));
  }
  kprintf("Done\n");
  return seL4_NoError;
}

Expected<std::shared_ptr<Thread>, seL4_Error>
ObjectFactory::createThread(seL4_Word tcbBadge, Thread::EntryPoint entryPoint,
                            seL4_CPtr apiEndpoint) {
  auto tcbOrErr = _untypedPool.allocObject(seL4_TCBObject);
  if (!tcbOrErr) {
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbOrErr.error);
  }
  auto thread = new Thread(tcbOrErr.value, entryPoint);
  if (!thread) {
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(
        seL4_NotEnoughMemory);
  }
  thread->badge = tcbBadge;
  auto tcbStackOrErr = _vmSpace.allocRangeAnywhere(1, seL4_ReadWrite);
  if (!tcbStackOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbStackOrErr.error);
  }
  thread->tcbStackAddr = tcbStackOrErr.value.vaddr;

  auto tcbTlsOrErr = _vmSpace.allocRangeAnywhere(1, seL4_ReadWrite);
  if (!tcbTlsOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbTlsOrErr.error);
  }

  auto tcbIPCOrErr = _vmSpace.allocIPCBuffer(seL4_ReadWrite);
  if (!tcbIPCOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbIPCOrErr.error);
  }

  auto err = seL4_TCB_SetTLSBase(thread->_tcb, tcbTlsOrErr.value.vaddr);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }

  seL4_Word prio = seL4_MaxPrio;
  err = thread->setPriority(prio);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
  assert(thread->getPriority() == prio);
  auto tcbEndpointSlotOrErr = getFreeSlot();
  assert(tcbEndpointSlotOrErr);
  if (!tcbEndpointSlotOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(
        tcbEndpointSlotOrErr.error);
  }
  seL4_SlotPos tcbEndpointSlot = tcbEndpointSlotOrErr.value;
  err = seL4_CNode_Mint(seL4_CapInitThreadCNode, tcbEndpointSlot, seL4_WordBits,
                        seL4_CapInitThreadCNode, apiEndpoint, seL4_WordBits,
                        seL4_AllRights, thread->badge);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    _untypedPool.releaseSlot(tcbEndpointSlotOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
  thread->endpoint = tcbEndpointSlot;
  seL4_Word faultEP = thread->endpoint;
  seL4_Word cspaceRootData = 0;
  seL4_Word vspaceRootData = 0;
  err = seL4_TCB_SetSpace(thread->_tcb, faultEP, seL4_CapInitThreadCNode,
                          cspaceRootData, seL4_CapInitThreadVSpace,
                          vspaceRootData);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    _untypedPool.releaseSlot(tcbEndpointSlotOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }

  return success<std::shared_ptr<Thread>, seL4_Error>(
      std::shared_ptr<Thread>(thread));
}

ObjectFactory::ObjectOrError ObjectFactory::createEndpoint() {
  return _untypedPool.allocObject(seL4_EndpointObject);
}

ObjectFactory::ObjectOrError ObjectFactory::createNotification() {
  return _untypedPool.allocObject(seL4_NotificationObject);
}

ObjectFactory::SlotOrError ObjectFactory::getFreeSlot() {
  return _untypedPool.getFreeSlot();
}

void ObjectFactory::releaseObject(seL4_CPtr) {
  kprintf("ObjectFactory::releaseObject does currently nothing\n");
}

void ObjectFactory::releaseSlot(seL4_SlotPos pos) {
  _untypedPool.releaseSlot(pos);
}