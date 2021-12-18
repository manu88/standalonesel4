#pragma once
#include "../lib/expected.hpp"
#include "../lib/vector.hpp"
#include "PCIBlk.hpp"
#include "PCIScanner.hpp"

class ObjectFactory;
class PageTable;

class PlatformExpert {
public:
  struct DMARange{
    DMARange(int){}

    void* virt = nullptr;
    uintptr_t phys = 0;
  };

  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;
  using DMARangeOrError = Expected<DMARange, seL4_Error>;
  bool init(ObjectFactory *factory, PageTable* pt);
  void print() const noexcept;

  SlotOrError issuePortRange(seL4_Word first_port, seL4_Word last_port);

  DMARangeOrError allocDMARange(size_t size);
  void releaseDMARange(DMARange&);
  
  seL4_Error doPowerOff();
private:
  void tryAssociatePCIDrivers();
  PCIScanner _pciScanner;
  ObjectFactory *_factory = nullptr;
  PageTable* _pt = nullptr;
  PCIBlk _pciblkDriver;
};