#pragma once
#include <utility>
#ifdef UNIT_TESTS
#include <stddef.h>
// XXX: create a sel4 stub header for unit tests
typedef size_t seL4_Word;
typedef size_t seL4_CPtr;
typedef int seL4_Error;
#define seL4_NoError 0
#define seL4_InvalidArgument 1
#define seL4_InvalidCapability 2
#define seL4_IllegalOperation 3
#define seL4_RangeError 4
#define seL4_AlignmentError 5
#define seL4_FailedLookup 6
#define seL4_TruncatedMessage 7
#define seL4_DeleteFirst 8
#define seL4_RevokeFirst 9
#define seL4_NotEnoughMemory 10
struct seL4_CapRights_t {
  int words[1];
};
#define seL4_ReadWrite seL4_CapRights_t()
#else
#include "sel4.hpp"
#include <stddef.h>
#endif
#include "Platform.hpp"
#include "lib/expected.hpp"
#include "lib/vector.hpp"

struct VMSpaceDelegate {
  virtual seL4_Error mapPage(seL4_Word, seL4_CapRights_t, seL4_Word &) {
    return seL4_IllegalOperation;
  }
};

struct VMSpace {
  enum RootServerLayout // Layout of root server, not other processes!!
  { AddressTables = 0x8000000000,
    ReservedVaddr = 0x8000001000, // size is KmallocReservedPages pages
  };

  struct Reservation {

    Reservation(size_t numPages) : numPages(numPages) {}

    Reservation(seL4_Word vaddr, size_t numPages, seL4_CapRights_t rights)
        : vaddr(vaddr), numPages(numPages), rights(rights) {}

    Reservation split(size_t afterPage) {
      if (pageCap != 0) {
        return Reservation(0);
      }
      if (!isValid()) {
        return Reservation(0);
      }
      if (afterPage >= (numPages - 1)) {
        return Reservation(0);
      }
      auto newRes = Reservation(numPages - (afterPage + 1));
      numPages -= newRes.numPages;
      newRes.vaddr = vaddr + (numPages * PAGE_SIZE);
      return newRes;
    }

    bool isValid() const noexcept { return numPages > 0; }

    seL4_Word vaddr = 0;
    size_t numPages = 0;
    seL4_CapRights_t rights = seL4_ReadWrite;
    seL4_CPtr pageCap = 0;

    bool inRange(seL4_Word addr) const noexcept {
      return addr >= vaddr && addr < (vaddr + numPages * PAGE_SIZE);
    }

    bool operator==(const Reservation &rhs) {
      return vaddr == rhs.vaddr && numPages == rhs.numPages &&
             rights.words[0] == rhs.rights.words[0] && pageCap == rhs.pageCap;
    }
    bool operator!=(const Reservation &rhs) { return !(*this == rhs); }
  };

  using ReservationOrError = Expected<Reservation, seL4_Error>;
  using ReservationSlot = std::pair<ssize_t, Reservation>;

  VMSpace() = delete;
  VMSpace(seL4_Word start);
  ReservationOrError
  allocRangeAnywhere(size_t numPages, seL4_CapRights_t rights = seL4_ReadWrite);
  seL4_Error deallocReservation(const Reservation &r);

  bool pageIsReserved(seL4_Word addr) const noexcept;
  void print() const noexcept;
  bool mapPage(seL4_Word addr);

  ReservationSlot getReservationForAddress(seL4_Word addr) const noexcept;
  size_t reservationCount() const noexcept { return _reservations.size(); }

  VMSpaceDelegate *delegate = nullptr;

private:
  seL4_Word currentVirtualAddress = 0;
  seL4_CPtr _vspace = 0;
  vector<Reservation> _reservations;
};