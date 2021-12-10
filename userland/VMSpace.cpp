#include "VMSpace.hpp"
#include "runtime.h"
#include "Platform.hpp"

VMSpace::VMSpace(seL4_Word start): currentVirtualAddress(start){}

VMSpace::Reservation VMSpace::allocRangeAnywhere(size_t numPages){
  printf("VMSpace::allocRangeAnywhere nPages %zi start is %X\n", numPages, currentVirtualAddress);

  VMSpace::Reservation r = {.vaddr = currentVirtualAddress, .numPages = numPages};
  _reservations.push_back(r);
  currentVirtualAddress += numPages* PAGE_SIZE;
  return r;   
}


void VMSpace::print() const noexcept{
  printf("--> VMSpace desc\n");
  printf("currentVirtualAddress is %X\n", currentVirtualAddress);
  printf("reservations:\n");
  for(const auto &res :  _reservations){
    printf("vaddr=%X size=%zi pages cap=%X rights=%s\n", res.vaddr, res.numPages, res.pageCap, seL4::rightsStr(res.rights));
  }
  printf("<--\n");
}