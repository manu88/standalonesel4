#include "VMSpace.hpp"
#ifdef UNIT_TESTS
#include <cstdio>
#else
#include "runtime.h"
#endif
#include "Platform.hpp"

VMSpace::VMSpace(seL4_Word start) : currentVirtualAddress(start) {}

VMSpace::ReservationOrError VMSpace::allocRangeAnywhere(size_t numPages,
                                                        seL4_CapRights_t rights,
                                                        bool isIPCBuffer) {
  printf("VMSpace::allocRangeAnywhere nPages %zi start is %X\n", numPages,
         currentVirtualAddress);
  VMSpace::Reservation r = Reservation(currentVirtualAddress, numPages, rights);
  r.isIPCBuffer = isIPCBuffer;
  _reservations.push_back(r);
  currentVirtualAddress += numPages * PAGE_SIZE;
  return success<Reservation, seL4_Error>(r);
}

seL4_Error VMSpace::deallocReservation(const VMSpace::Reservation &r) {
  printf("VMSpace::deallocReservation vaddr %X\n", r.vaddr);
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

bool VMSpace::mapPage(seL4_Word addr) {
  printf("request add addr %X\n", addr);
  auto resSlot = getReservationForAddress(addr);
  if (resSlot.first == -1) {
    return false;
  }
  if (resSlot.second.pageCap > 0) {
    return false; // already mapped
  }
  if (resSlot.second.numPages == 1) {
    // no split needed
    printf("MAP REquest at addr %X\n", resSlot.second.vaddr);
    seL4_Word cap = 0;
    auto err =
        delegate->mapPage(resSlot.second.vaddr, resSlot.second.rights, cap);
    if (err == seL4_NoError) {
      _reservations[resSlot.first].pageCap = cap;
    }
    return err == seL4_NoError;
  } else {
    seL4_Word relativeAddr = addr - resSlot.second.vaddr;
    size_t containingPageNum = (relativeAddr % PAGE_SIZE) - 1;
    printf("relativeAddr=%X page %zi\n", relativeAddr, containingPageNum);
    auto splitRes1 = resSlot.second.split(containingPageNum);
    assert(splitRes1.isValid());
    printf("Splittee is numpages=%zi\n", resSlot.second.numPages);
    _reservations[resSlot.first] = resSlot.second;
    printf("splitted 1 at addr %X numPages %zi\n", splitRes1.vaddr,
           splitRes1.numPages);
    if (splitRes1.numPages == 1) {
      seL4_Word cap = 0;
      auto err = delegate->mapPage(splitRes1.vaddr, splitRes1.rights, cap);
      if (err == seL4_NoError) {
        splitRes1.pageCap = cap;
        _reservations.push_back(splitRes1);
      }
      return err == seL4_NoError;
    }
    auto splitRes2 = splitRes1.split(0);
    if (splitRes2.isValid()) {
      printf("splitted 2 at addr %X numPages %zi\n", splitRes2.vaddr,
             splitRes2.numPages);
      seL4_Word cap = 0;
      auto err = delegate->mapPage(splitRes2.vaddr, splitRes2.rights, cap);
      if (err == seL4_NoError) {
        splitRes2.pageCap = cap;
        _reservations.push_back(splitRes2);
        _reservations.push_back(splitRes2);
      }
      return err == seL4_NoError;
    }
  }
  return false;
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
  printf("--> VMSpace desc\n");
  printf("currentVirtualAddress is %X\n", currentVirtualAddress);
  printf("reservations:\n");
  for (const auto &res : _reservations) {
    printf("vaddr=%X size=%zi pages cap=%X rights=%X isIPC=%i\n", res.vaddr,
           res.numPages, res.pageCap, res.rights, res.isIPCBuffer);
  }
  printf("<--\n");
}