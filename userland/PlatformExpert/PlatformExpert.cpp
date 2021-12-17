#include "PlatformExpert.hpp"
#include "../ObjectFactory.hpp"
#include "../runtime.h"
#include <stdint.h>
#include "DriverBase.hpp"


bool PlatformExpert::init(ObjectFactory *factory) {
  _factory = factory;
  kprintf("PlatformExpert::init for x86_64\n");
  kprintf("Issue PCI Config range req\n");
  auto pciConfigAddressSlotOrErr = _factory->getFreeSlot();
  assert(pciConfigAddressSlotOrErr);
  auto pciDataAddressSlotOrErr = _factory->getFreeSlot();
  assert(pciDataAddressSlotOrErr);

  auto err = seL4_X86_IOPortControl_Issue(
      seL4_CapIOPortControl, PCI_CONFIG_ADDRESS, PCI_CONFIG_ADDRESS + 3,
      seL4_CapInitThreadCNode, pciConfigAddressSlotOrErr.value, seL4_WordBits);
  assert(err == seL4_NoError);

  err = seL4_X86_IOPortControl_Issue(
      seL4_CapIOPortControl, PCI_CONFIG_DATA, PCI_CONFIG_DATA + 3,
      seL4_CapInitThreadCNode, pciDataAddressSlotOrErr.value, seL4_WordBits);
  assert(err == seL4_NoError);

  _pciScanner.init(pciConfigAddressSlotOrErr.value,
                   pciDataAddressSlotOrErr.value);

  _pciScanner.scan();
  print();
  tryAssociatePCIDrivers();
  return true;
}

void PlatformExpert::print() const noexcept {
  kprintf("Got %zi PCI devices\n", _pciScanner.getDevices().size());
  for (const auto &dev : _pciScanner.getDevices()) {
    dev.print();
  }
}

void PlatformExpert::tryAssociatePCIDrivers(){
  kprintf("tryAssociatePCIDrivers\n");
  for (const auto &dev : _pciScanner.getDevices()) {
      if(_pciblkDriver.probe(dev)){
        kprintf("probing sucessful for device %s %s and driver %s\n", dev.vendorName(), dev.deviceName(), _pciblkDriver.getName());
        _pciblkDriver.addDevice(dev, 0);
      }
  }
}