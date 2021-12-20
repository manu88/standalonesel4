#include "PlatformExpert.hpp"
#include "../ObjectFactory.hpp"
#include "../PageTable.hpp"
#include "../runtime.h"
#include "DriverBase.hpp"
#include <stdint.h>

bool PlatformExpert::init(ObjectFactory *factory, PageTable* pt) {
  _factory = factory;
  _pt = pt;
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

PlatformExpert::SlotOrError PlatformExpert::issuePortRangeWithSize(seL4_Word port, size_t range){
  return issuePortRange(port, port + range -1);
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

PlatformExpert::DMARangeOrError PlatformExpert::allocDMARange(size_t size, uint16_t deviceID ,uint16_t domainID){
  auto slotOrErr = _factory->getFreeSlot();
  if (!slotOrErr) {
    return unexpected<PlatformExpert::DMARange, seL4_Error>(slotOrErr.error);
  }
  auto slot = slotOrErr.value;
  seL4_Word badge = ((uint32_t)domainID << 16) | (uint32_t)deviceID;
  assert(seL4_CapIOSpace);
  kprintf("Test seL4_CNode_Mint dma range thing seL4_CapIOSpace=0X%X dest slot 0X%X\n", seL4_CapIOSpace, slot);
  auto err = seL4_CNode_Mint(seL4_CapInitThreadCNode, slot, seL4_WordBits, // dest
                            seL4_CapInitThreadCNode, seL4_CapIOSpace, seL4_WordBits, // src
                            seL4_AllRights, badge);
  if(err != seL4_NoError){
    _factory->releaseSlot(slot);
  }
  return unexpected<PlatformExpert::DMARange, seL4_Error>(err);
}

void PlatformExpert::releaseDMARange(DMARange&){

}


seL4_Error PlatformExpert::doPowerOff(){
  // QEMU specific! see https://github.com/manu88/Sofa/blob/fce20513a68c2e50a9b5185fc048b4fee0183eac/projects/Sofa/kernel_task/src/Drivers/X86/PCI.c#L40
  // and https://wiki.osdev.org/QEMU_fw_cfg on how to check is running under qemu
  auto stopSlot = issuePortRange(0x604, 0x604 +1);
  if(!stopSlot){
    return stopSlot.error;
  }
  return seL4_X86_IOPort_Out16(stopSlot.value, 0x604, 0x2000);
}

