#include "PlatformExpert.hpp"
#include "../ObjectFactory.hpp"
#include "../runtime.h"
#include "DriverBase.hpp"
#include <stdint.h>

bool PlatformExpert::init(ObjectFactory *factory) {
  _factory = factory;
  kprintf("PlatformExpert::init for x86_64\n");
  kprintf("Issue PCI Config range req\n");

  auto pciConfigAddressSlotOrErr =
      issuePortRange(PCI_CONFIG_ADDRESS, PCI_CONFIG_ADDRESS + 3);
  assert(pciConfigAddressSlotOrErr);
  auto pciDataAddressSlotOrErr =
      issuePortRange(PCI_CONFIG_DATA, PCI_CONFIG_DATA + 3);
  assert(pciDataAddressSlotOrErr);
  _pciScanner.init(pciConfigAddressSlotOrErr.value,
                   pciDataAddressSlotOrErr.value);
  _pciScanner.scan();
  print();
  tryAssociatePCIDrivers();
  return true;
}

PlatformExpert::SlotOrError
PlatformExpert::issuePortRange(seL4_Word first_port, seL4_Word last_port) {
  auto slotOrErr = _factory->getFreeSlot();
  if (!slotOrErr) {
    return slotOrErr;
  }
  auto err = seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl, first_port,
                                          last_port, seL4_CapInitThreadCNode,
                                          slotOrErr.value, seL4_WordBits);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
  }
  return slotOrErr;
}

void PlatformExpert::print() const noexcept {
  kprintf("Got %zi PCI devices\n", _pciScanner.getDevices().size());
  for (const auto &dev : _pciScanner.getDevices()) {
    dev.print();
  }
}

void PlatformExpert::tryAssociatePCIDrivers() {
  kprintf("tryAssociatePCIDrivers\n");
  for (const auto &dev : _pciScanner.getDevices()) {
    if (_pciblkDriver.probe(dev)) {
      kprintf("probing sucessful for device %s %s and driver %s\n",
              dev.vendorName(), dev.deviceName(), _pciblkDriver.getName());
      _pciblkDriver.addDevice(*this, dev);
    }
  }
}