#pragma once
#include "../lib/expected.hpp"
#include "../lib/vector.hpp"
#include "PCIBlk.hpp"
#include "PCIScanner.hpp"

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

  using SlotOrError = Expected<seL4_SlotPos, seL4_Error>;
  using DMARangeOrError = Expected<DMARange, seL4_Error>;
  bool init(ObjectFactory *factory, PageTable* pt);
  void print() const noexcept;

  SlotOrError issuePortRange(seL4_Word first_port, seL4_Word last_port);
  SlotOrError issuePortRangeWithSize(seL4_Word port, size_t range);

  SlotOrError getMSIHandle(const PCIDevice& dev, seL4_Word handle, seL4_Word vector);
  SlotOrError getIRQHandle(const PCIDevice& dev);
  SlotOrError getIOAPICIRQHandle(const PCIDevice& dev);

  DMARangeOrError allocDMARange(size_t size);
  void releaseDMARange(DMARange&);

  bool registerBlockDevice(BlockDevice *dev);

  const vector<BlockDevice*>& getBlockDevices() const noexcept{
    return _devices;
  }

  seL4_Error doPowerOff();
  PCIBlk _pciblkDriver; // TEMP
private:
  void tryAssociatePCIDrivers();
  PCIScanner _pciScanner;
  ObjectFactory *_factory = nullptr;
  PageTable* _pt = nullptr;

  vector<BlockDevice*> _devices;
};