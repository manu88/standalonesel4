#pragma once
#include <utility>
#ifdef UNIT_TESTS
#include "sel4_mock.hpp"
#else
#include "sel4.hpp"
#include <stddef.h>
#endif
#include "Platform.hpp"
#include "lib/expected.hpp"
#include "lib/vector.hpp"
#include <sys/types.h> // size_t

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
      newRes.isIPCBuffer = isIPCBuffer;
      return newRes;
    }

    bool isValid() const noexcept { return numPages > 0; }

    seL4_Word vaddr = 0;
    size_t numPages = 0;
    seL4_CapRights_t rights = seL4_ReadWrite;
    seL4_CPtr pageCap = 0;
    bool isIPCBuffer = false;

    bool inRange(seL4_Word addr) const noexcept {
      return addr >= vaddr && addr < (vaddr + numPages * PAGE_SIZE);
    }

    bool operator==(const Reservation &rhs) {
      return vaddr == rhs.vaddr && numPages == rhs.numPages &&
             rights.words[0] == rhs.rights.words[0] && pageCap == rhs.pageCap &&
             isIPCBuffer == rhs.isIPCBuffer;
    }
    bool operator!=(const Reservation &rhs) { return !(*this == rhs); }
  };

  using ReservationOrError = Expected<Reservation, seL4_Error>;
  using ReservationSlot = std::pair<ssize_t, Reservation>;

  VMSpace() = delete;
  VMSpace(seL4_Word start);

  ReservationOrError
  allocRangeAnywhere(size_t numPages, seL4_CapRights_t rights = seL4_ReadWrite,
                     bool isIPCBuffer = false);

  ReservationOrError allocIPCBuffer(seL4_CapRights_t rights = seL4_ReadWrite) {
    return allocRangeAnywhere(1, rights, true);
  }

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