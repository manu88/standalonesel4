#include "ObjectFactory.hpp"
#include "InitialUntypedPool.hpp"
#include "PageTable.hpp"
#include "runtime.h"

ObjectFactory::ObjectFactory(InitialUntypedPool &untypedPool, PageTable &pt,
                             seL4_Word currentVirtualAddress)
    : currentVirtualAddress(currentVirtualAddress), _untypedPool(untypedPool),
      _pt(pt) {}


Expected<std::shared_ptr<Thread>, seL4_Error> ObjectFactory::createThread(seL4_Word tcbBadge,
                                            Thread::EntryPoint entryPoint,
                                            seL4_CPtr apiEndpoint){
  auto tcbOrErr = _untypedPool.allocObject(seL4_TCBObject);
  if (!tcbOrErr) {
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbOrErr.error);
  }
  assert(tcbOrErr);
  auto thread = new Thread(tcbOrErr.value, entryPoint);
  if(!thread){
        return unexpected<std::shared_ptr<Thread>, seL4_Error>(seL4_NotEnoughMemory);
  }
  thread->badge = tcbBadge;
  seL4_Word faultEP = 0;
  seL4_Word cspaceRootData = 0;
  seL4_Word vspaceRootData = 0;
  seL4_Error err = seL4_TCB_SetSpace(thread->_tcb, faultEP,
                                     seL4_CapInitThreadCNode, cspaceRootData,
                                     seL4_CapInitThreadVSpace, vspaceRootData);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
  // to release
  thread->tcbStackAddr = currentVirtualAddress;
  auto tcbStackOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
  if (!tcbStackOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbStackOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  seL4_Word tlsAddr = currentVirtualAddress;
  auto tcbTlsOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
  if (!tcbTlsOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbTlsOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  seL4_Word tcbIPC = currentVirtualAddress;
  auto tcbIPCOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
  if (!tcbIPCOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbIPCOrErr.error);
  }
  currentVirtualAddress += PAGE_SIZE;

  err = seL4_TCB_SetTLSBase(thread->_tcb, tlsAddr);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    _pt.unmapPage(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }

  err = seL4_TCB_SetIPCBuffer(thread->_tcb, tcbIPC, tcbIPCOrErr.value);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    _pt.unmapPage(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }

  seL4_Word prio = seL4_MaxPrio;
  err = thread->setPriority(prio);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    _pt.unmapPage(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
  assert(thread->getPriority() == prio);
  auto tcbEndpointSlotOrErr = _untypedPool.getFreeSlot();
  assert(tcbEndpointSlotOrErr);
  if (!tcbEndpointSlotOrErr) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    _pt.unmapPage(tcbIPCOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(tcbEndpointSlotOrErr.error);
  }
  seL4_SlotPos tcbEndpointSlot = tcbEndpointSlotOrErr.value;
  err = seL4_CNode_Mint(seL4_CapInitThreadCNode, tcbEndpointSlot, seL4_WordBits,
                        seL4_CapInitThreadCNode, apiEndpoint, seL4_WordBits,
                        seL4_AllRights, thread->badge);
  if (err != seL4_NoError) {
    _untypedPool.releaseObject(tcbOrErr.value);
    _pt.unmapPage(tcbStackOrErr.value);
    _pt.unmapPage(tcbTlsOrErr.value);
    _pt.unmapPage(tcbIPCOrErr.value);
    _untypedPool.releaseSlot(tcbEndpointSlotOrErr.value);
    return unexpected<std::shared_ptr<Thread>, seL4_Error>(err);
  }
  thread->endpoint = tcbEndpointSlot;
  return success<std::shared_ptr<Thread>, seL4_Error>(std::shared_ptr<Thread>(thread));
}

ObjectFactory::ObjectOrError ObjectFactory::createEndpoint() {
  return _untypedPool.allocObject(seL4_EndpointObject);
}

ObjectFactory::ObjectOrError ObjectFactory::createNotification() {
  return _untypedPool.allocObject(seL4_NotificationObject);
}
