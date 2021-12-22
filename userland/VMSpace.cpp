#include "VMSpace.hpp"
#ifdef UNIT_TESTS
#include <cstdio>
#define kprintf printf
#else
#include "runtime.h"
#endif
#include "Platform.hpp"
#include "klog.h"


#ifdef UNIT_TESTS
struct seL4_X86_Page_GetAddress_Ret{
  seL4_Word paddr;
};
static seL4_X86_Page_GetAddress_Ret seL4_X86_Page_GetAddress(seL4_Word){return {.paddr = 0};}
#endif
/*static*/ VMSpace::PhysicalAddressOrError VMSpace::Reservation::getPhysicalAddr(seL4_Word cap){
  if(cap){
    auto r = seL4_X86_Page_GetAddress(cap);
    return success<seL4_Word, seL4_Error>(r.paddr);
  }
  return unexpected<seL4_Word, seL4_Error>(seL4_InvalidCapability);
}

VMSpace::VMSpace(seL4_Word start) : currentVirtualAddress(start) {}

VMSpace::ReservationOrError VMSpace::allocRangeAnywhere(size_t numPages,
                                                        seL4_CapRights_t rights,
                                                        VMSpace::MemoryType type) {
  VMSpace::Reservation r = Reservation(currentVirtualAddress, numPages, rights);
  r.type = type;
  _reservations.push_back(r);
  currentVirtualAddress += numPages * PAGE_SIZE;
  return success<Reservation, seL4_Error>(r);
}

seL4_Error VMSpace::deallocReservation(const VMSpace::Reservation &r) {
  kprintf("VMSpace::deallocReservation vaddr %X\n", r.vaddr);
  return seL4_NoError;
}

bool VMSpace::pageIsReserved(seL4_Word addr) const noexcept {
  for (const auto &res : _reservations) {
    if (res.inRange(addr)) {
      return true;
    }
  }
  return false;
}

VMSpace::PhysicalAddressOrError VMSpace::mapPage(seL4_Word addr) {
// FIXME: overly complicated! need some serious refactoring.
  auto resSlot = getReservationForAddress(addr);
  if (resSlot.first == -1) {
    return unexpected<seL4_Word, seL4_Error>(seL4_InvalidArgument);
  }
  if (resSlot.second.pageCap > 0) {
    return unexpected<seL4_Word, seL4_Error>(seL4_InvalidArgument); // already mapped
  }
  if (resSlot.second.numPages == 1) {
    // no split needed
    seL4_Word cap = 0;
    auto err =
        delegate->mapPage(resSlot.second.vaddr, resSlot.second.rights, cap);
    if (err == seL4_NoError) {
      _reservations[resSlot.first].pageCap = cap;
      return success<seL4_Word, seL4_Error>(VMSpace::Reservation::getPhysicalAddr(cap).value);
    }
    return unexpected<seL4_Word, seL4_Error>(err);
  } else {
    bool mapFirstSeg = true;
    seL4_Word relativeAddr = addr - resSlot.second.vaddr;
    size_t containingPageNum = (relativeAddr % PAGE_SIZE);
    if(containingPageNum != 0){
      mapFirstSeg = false;
      containingPageNum-=1;
    }
    auto splitRes1 = resSlot.second.split(containingPageNum);
    if(!splitRes1.isValid()){
      kprintf("splitRes1 invalid\n");
      print();
    }
    assert(splitRes1.isValid());

    if(mapFirstSeg){
      seL4_Word cap = 0;
      auto err = delegate->mapPage(resSlot.second.vaddr, resSlot.second.rights, cap);
      if(err == seL4_NoError){
        resSlot.second.pageCap = cap;
        _reservations[resSlot.first] = resSlot.second;
        _reservations.push_back(splitRes1);
      }
      if(err != seL4_NoError){
        return unexpected<seL4_Word, seL4_Error>(err);
      }
      return success<seL4_Word, seL4_Error>(VMSpace::Reservation::getPhysicalAddr(cap).value);
    }
    if (splitRes1.numPages == 1) {
      seL4_Word cap = 0;
      auto err = delegate->mapPage(splitRes1.vaddr, splitRes1.rights, cap);
      if (err == seL4_NoError) {
        _reservations[resSlot.first] = resSlot.second;
        splitRes1.pageCap = cap;
        _reservations.push_back(splitRes1);
      }
      if(err != seL4_NoError){
        return unexpected<seL4_Word, seL4_Error>(err);
      }
      return success<seL4_Word, seL4_Error>(VMSpace::Reservation::getPhysicalAddr(cap).value);
    }
    auto splitRes2 = splitRes1.split(0);
    if (splitRes2.isValid()) {
      seL4_Word cap = 0;
      auto err = delegate->mapPage(splitRes2.vaddr, splitRes2.rights, cap);
      if (err == seL4_NoError) {
        _reservations[resSlot.first] = resSlot.second;
        splitRes2.pageCap = cap;
        _reservations.push_back(splitRes2);
        _reservations.push_back(splitRes2);
      }
      if(err != seL4_NoError){
        return unexpected<seL4_Word, seL4_Error>(err);
      }
      return success<seL4_Word, seL4_Error>(VMSpace::Reservation::getPhysicalAddr(cap).value);
    }
  }
  assert(0);
  return unexpected<seL4_Word, seL4_Error>(seL4_InvalidArgument);
}

VMSpace::ReservationSlot
VMSpace::getReservationForAddress(seL4_Word addr) const noexcept {
  size_t i = 0;
  for (const auto &res : _reservations) {
    if (res.inRange(addr)) {
      return std::make_pair(i, res);
      // success<Reservation, seL4_Error>(res);
    }
    i++;
  }
  return std::make_pair(-1, Reservation(0));
  // unexpected<Reservation, seL4_Error>(seL4_RangeError);
}

void VMSpace::print() const noexcept {
  kprintf("--> VMSpace desc\n");
  kprintf("currentVirtualAddress is %X\n", currentVirtualAddress);
  kprintf("reservations:\n");

  auto typeToStr = [](VMSpace::MemoryType t) -> const char*{
    switch (t)
    {
    case VMSpace::MemoryType::Regular:
      return "Regular";
      break;
    case VMSpace::MemoryType::IPC:
      return "IPC";
      break;
    case VMSpace::MemoryType::DMA:
      return "DMA";
      break;
    default:
      break;
    }
    kprintf("typeToStr: undefined VMSpace::MemoryType value 0X%X\n", t);
    return "ERROR";
  };

  for (const auto &res : _reservations) {
    kprintf("vaddr=%X size=%zi pages cap=%X rights=%X type=%s", res.vaddr,
            res.numPages, res.pageCap, res.rights, typeToStr(res.type));
    auto physAddrOrErr = res.getPhysicalAddr();
    if(physAddrOrErr){
      kprintf(" paddr=0X%X", physAddrOrErr.value);
    }
    kprintf("\n");
  }
  kprintf("<--\n");
}