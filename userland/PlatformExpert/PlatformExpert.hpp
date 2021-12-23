#pragma once
#include "../lib/expected.hpp"
#include "../lib/vector.hpp"
#include "PCIBlk.hpp"
#include "PCIScanner.hpp"
#include "Pit.hpp"

class ObjectFactory;
class PageTable;
struct BlockDevice;

class PlatformExpert {
public:
  struct DMARange{
    DMARange(int = 0){}
    void* virt = nullptr;
    seL4_Word phys = 0;
  };
  struct IRQHandle{
    IRQHandle(seL4_SlotPos notif = 0, seL4_SlotPos irqCap = 0):
    notif(notif), irqCap(irqCap)
    {}
    seL4_SlotPos notif = 0;
    seL4_SlotPos irqCap = 0;
    seL4_Error ack();
  };
  using IRQHandleOrError = Expected<IRQHandle, seL4_Error>;
  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;
  using DMARangeOrError = Expected<DMARange, seL4_Error>;
  
  bool init(ObjectFactory *factory, PageTable* pt);
  void print() const noexcept;

  SlotOrError issuePortRange(seL4_Word first_port, seL4_Word last_port);
  SlotOrError issuePortRangeWithSize(seL4_Word port, size_t range);
  void dropPortRange(seL4_Word cap);

  IRQHandleOrError getMSIHandle(const PCIDevice& dev, seL4_Word handle, seL4_Word vector);
  IRQHandleOrError getIRQHandle(int irqLine);
  IRQHandleOrError getIOAPICIRQHandle(seL4_Word ioapic, seL4_Word vector, seL4_Word pin);

  DMARangeOrError allocDMARange(size_t size);
  void releaseDMARange(DMARange&);

  bool registerBlockDevice(BlockDevice *dev);

  const vector<BlockDevice*>& getBlockDevices() const noexcept{
    return _devices;
  }

  IRQHandle getPitIRQ() const noexcept{
    return IRQHandle(_pit.irqNotif, _pit.irqCap);
  }

  seL4_Error doPowerOff();
  PCIBlk _pciblkDriver; // TEMP
private:
  void tryAssociatePCIDrivers();
  PCIScanner _pciScanner;
  PIT _pit;
  ObjectFactory *_factory = nullptr;
  PageTable* _pt = nullptr;

  vector<BlockDevice*> _devices;
};