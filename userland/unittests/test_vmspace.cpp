#include "../VMSpace.hpp"
#include "tests.hpp"
#include <assert.h>
#include <cstdio>

struct TestVMSpaceDelegate : public VMSpaceDelegate {
  seL4_Error mapPage(seL4_Word, seL4_CapRights_t, seL4_Word &) override {
    return seL4_NoError;
  }
};

static void testReservations() {
  VMSpace::Reservation r0(0, 0, seL4_ReadWrite);
  assert(r0.isValid() == false);

  for (ssize_t i = 0; i < 10; i++) {
    auto splitr0 = r0.split(1);
    assert(splitr0.isValid() == false);
  }

  VMSpace::Reservation r1(0, 1, seL4_ReadWrite);
  assert(r1.isValid());

  for (ssize_t i = 0; i < 10; i++) {
    auto splitr0 = r1.split(1);
    assert(splitr0.isValid() == false);
  }

  VMSpace::Reservation r2(0, 2, seL4_ReadWrite);
  assert(r1.isValid());

  assert(r2.split(1).isValid() == false);

  auto r2split = r2.split(0);
  printf("r2 pages=%zi, new seg = %zi\n", r2.numPages, r2split.numPages);
  assert(r2split.isValid());
  assert(r2.numPages == 1);
  assert(r2.vaddr == 0);

  assert(r2split.numPages == 1);
  assert(r2split.vaddr == PAGE_SIZE);

  VMSpace::Reservation r3(0, 2, seL4_ReadWrite);
  r3.pageCap = 42; // can't split a reservation if a page is already mapped.
  assert(r3.isValid());
  assert(r3.split(0).isValid() == false);
}

int testVMSpace(void) {
  testReservations();
  const seL4_Word addrStart = 0x1000;
  const size_t numPages = 12;
  VMSpace a(addrStart);
  TestVMSpaceDelegate delegate;
  a.delegate = &delegate;
  assert(a.pageIsReserved(0) == false);
  auto res = a.allocRangeAnywhere(numPages);
  assert(a.pageIsReserved(addrStart - 1) == false);
  assert(res);
  a.print();
  for (size_t i = 0; i < numPages; i++) {
    size_t pageStart = addrStart + (i * PAGE_SIZE);
    for (size_t j = 0; j < PAGE_SIZE; j++) {
      assert(a.pageIsReserved(pageStart + j));
    }
  }

  auto reservationSlot = a.getReservationForAddress(addrStart + PAGE_SIZE);
  assert(reservationSlot.first != -1);
  assert(reservationSlot.second.isValid());

  auto countBeforeMapping = a.reservationCount();
  printf("Test Map page countBeforeMapping=%zi\n", a.reservationCount());
  auto ret = a.mapPage(addrStart + PAGE_SIZE + 1);
  assert(ret);
  a.print();
  printf("after, countBeforeMapping=%zi\n", a.reservationCount());
  assert(a.reservationCount() == countBeforeMapping + 2);
  return 0;
}