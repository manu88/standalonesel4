#include "ObjectFactory.hpp"
#include "InitialUntypedPool.hpp"
#include "PageTable.hpp"
#include "VMSpace.hpp"
#include "runtime.h"

ObjectFactory::ObjectFactory(InitialUntypedPool &untypedPool, PageTable &pt,
                             VMSpace &vm, seL4_Word currentVirtualAddress)
    : currentVirtualAddress(currentVirtualAddress), _untypedPool(untypedPool),
      _pt(pt), _vmSpace(vm) {}

Expected<std::shared_ptr<Thread>, seL4_Error>
ObjectFactory::createThread(seL4_Word tcbBadge, Thread::EntryPoint entryPoint,
                            seL4_CPtr apiEndpoint) {
  auto tcbOrErr = _untypedPool.allocObject(seL4_TCBObject);
  if (!tcbOrErr) {
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbOrErr.error);
  }
  assert(tcbOrErr);
  auto thread = new Thread(tcbOrErr.value, entryPoint);
  if (!thread) {
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(
        seL4_NotEnoughMemory);
  }
  thread->badge = tcbBadge;

  thread->tcbStackAddr = currentVirtualAddress;
  auto tcbStackOrErr = _vmSpace.allocRangeAnywhere(1, seL4_ReadWrite);
  if (!tcbStackOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbStackOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  seL4_Word tlsAddr = currentVirtualAddress;
  auto tcbTlsOrErr = _vmSpace.allocRangeAnywhere(1, seL4_ReadWrite);
  if (!tcbTlsOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbTlsOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  seL4_Word tcbIPC = currentVirtualAddress;
  auto tcbIPCOrErr = _vmSpace.allocIPCBuffer(seL4_ReadWrite);
  if (!tcbIPCOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbIPCOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  auto err = seL4_TCB_SetTLSBase(thread->_tcb, tlsAddr);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
#if 0
  err = seL4_TCB_SetIPCBuffer(thread->_tcb, tcbIPC, tcbIPCOrErr.value.pageCap);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _vmSpace.deallocReservation(tcbStackOrErr.value);
    _vmSpace.deallocReservation(tcbTlsOrErr.value);
    _vmSpace.deallocReservation(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
#endif
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
  auto tcbEndpointSlotOrErr = _untypedPool.getFreeSlot();
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
