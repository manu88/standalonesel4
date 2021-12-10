#pragma once
#include "sel4.hpp"
#include <stddef.h>
#include "lib/vector.hpp"

struct VMSpace
{
  struct Reservation{
    seL4_Word vaddr = 0;
    size_t numPages = 0;
    seL4_CapRights_t rights = seL4_ReadWrite;
    seL4_CPtr pageCap = 0;

  };
  enum RootServerLayout // Layout of root server, not other processes!!
  { AddressTables = 0x8000000000,
    ReservedVaddr = 0x8000001000, // size is KmallocReservedPages pages
  };

  VMSpace() = delete;
  VMSpace(seL4_Word start);
  Reservation allocRangeAnywhere(size_t numPages);

  void print() const noexcept;

private:
  seL4_Word currentVirtualAddress = 0;
  seL4_CPtr _vspace = 0;
  vector<Reservation> _reservations;
};