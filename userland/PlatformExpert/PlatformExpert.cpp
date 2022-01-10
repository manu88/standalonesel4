#include "PlatformExpert.hpp"
#include "../BlockDevice.hpp"
#include "../ObjectFactory.hpp"
#include "../PageTable.hpp"
#include "../Thread.hpp"
#include "../VMSpace.hpp"
#include "../klog.h"
#include "../runtime.h"
#include "DriverBase.hpp"
#include <stdint.h>

seL4_Error PlatformExpert::IRQHandle::ack() {
  return seL4_IRQHandler_Ack(irqCap);
}

bool PlatformExpert::init(ObjectFactory *factory, PageTable *pt) {
  _factory = factory;
  _pt = pt;
  kprintf("PlatformExpert::init for x86_64\n");
  kprintf("PlatformExpert::init Pit\n");
  assert(_pit.init(*this, _factory));
#define IRQS_PER_SECOND 100
  assert(_pit.setTimeout(NS_IN_S / IRQS_PER_SECOND, true));
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
  tryAssociatePCIDrivers();
  return true;
}

void PlatformExpert::dropPortRange(seL4_Word cap) {
  _factory->releaseSlot(cap);
}

PlatformExpert::SlotOrError
PlatformExpert::issuePortRangeWithSize(seL4_Word port, size_t range) {
  return issuePortRange(port, port + range - 1);
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

PlatformExpert::IRQHandleOrError
PlatformExpert::getIOAPICIRQHandle(seL4_Word ioapic, seL4_Word vector,
                                   seL4_Word pin) {
  auto slotOrErr = _factory->getFreeSlot();
  if (!slotOrErr) {
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(slotOrErr.error);
  }
  enum { IRQ_EDGE = 0, IRQ_LEVEL = 1 };
  enum { IRQ_HIGH = 0, IRQ_LOW = 1 };
  seL4_Word level = /*(pin < 16) ? IRQ_EDGE :*/ IRQ_LEVEL;
  seL4_Word polarity = /*(pin < 16) ? IRQ_HIGH :*/ IRQ_LOW;
  auto err = seL4_IRQControl_GetIOAPIC(
      seL4_CapIRQControl, seL4_CapInitThreadCNode, slotOrErr.value,
      seL4_WordBits, ioapic, pin, level, polarity, vector);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(err);
  }
  auto notifOrErr = _factory->createNotification();
  if (!notifOrErr) {
    _factory->releaseSlot(slotOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(notifOrErr.error);
  }
  err = seL4_IRQHandler_SetNotification(slotOrErr.value, notifOrErr.value);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
    _factory->releaseObject(notifOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(err);
  }
  return success<PlatformExpert::IRQHandle, seL4_Error>(
      PlatformExpert::IRQHandle(notifOrErr.value, slotOrErr.value));
}

PlatformExpert::IRQHandleOrError PlatformExpert::getIRQHandle(int irqLine) {
  auto slotOrErr = _factory->getFreeSlot();
  if (!slotOrErr) {
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(slotOrErr.error);
  }
  auto err =
      seL4_IRQControl_Get(seL4_CapIRQControl, irqLine, seL4_CapInitThreadCNode,
                          slotOrErr.value, seL4_WordBits);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(err);
  }
  auto notifOrErr = _factory->createNotification();
  if (!notifOrErr) {
    _factory->releaseSlot(slotOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(notifOrErr.error);
    // return unexpected<seL4_SlotPos, seL4_Error>(notifOrErr.error);
  }
  err = seL4_IRQHandler_SetNotification(slotOrErr.value, notifOrErr.value);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
    _factory->releaseObject(notifOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(err);
  }
  return success<PlatformExpert::IRQHandle, seL4_Error>(
      PlatformExpert::IRQHandle(notifOrErr.value, slotOrErr.value));
}

PlatformExpert::IRQHandleOrError
PlatformExpert::getMSIHandle(const PCIDevice &dev, seL4_Word handle,
                             seL4_Word vector) {
  auto slotOrErr = _factory->getFreeSlot();
  if (!slotOrErr) {
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(slotOrErr.error);
  }
  auto err = seL4_IRQControl_GetMSI(seL4_CapIRQControl, seL4_CapInitThreadCNode,
                                    slotOrErr.value, seL4_WordBits, dev.bus,
                                    dev.slot, dev.fun, handle, vector);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
  }
  auto notifOrErr = _factory->createNotification();
  if (!notifOrErr) {
    _factory->releaseSlot(slotOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(notifOrErr.error);
  }
  err = seL4_IRQHandler_SetNotification(slotOrErr.value, notifOrErr.value);
  if (err != seL4_NoError) {
    _factory->releaseSlot(slotOrErr.value);
    _factory->releaseObject(notifOrErr.value);
    return unexpected<PlatformExpert::IRQHandle, seL4_Error>(err);
  }
  return success<PlatformExpert::IRQHandle, seL4_Error>(
      PlatformExpert::IRQHandle(notifOrErr.value, slotOrErr.value));
}

void PlatformExpert::print() const noexcept {
  kprintf("Got %zi PCI devices\n", _pciScanner.getDevices().size());
  for (const auto &dev : _pciScanner.getDevices()) {
    dev.print();
  }
  for (const auto &dev : _devices) {
    kprintf("Got a registered device %p\n", (void *)dev);
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

PlatformExpert::DMARangeOrError PlatformExpert::allocDMARange(size_t size) {
  size_t numPages = (size / PAGE_SIZE) + 1;
  auto callingThread = Thread::getCurrent();
  assert(Thread::calledFromMain()); // XXX Right now we use main thread: be sure
                                    // to use Syscalls to issue this command when called from other threads
  assert(callingThread->vmspace != nullptr);
  auto resOrErr = callingThread->vmspace->allocRangeAnywhere(
      numPages, seL4_ReadWrite, VMSpace::MemoryType::DMA);

  if (!resOrErr) {
    return unexpected<PlatformExpert::DMARange, seL4_Error>(resOrErr.error);
  }
  seL4_Word startVirt = 0;
  seL4_Word startPhys = 0;
  for (size_t i = 0; i < numPages; i++) {
    size_t mapAddr = resOrErr.value.vaddr + (i * PAGE_SIZE);
    auto physicalAddressOrError = callingThread->vmspace->mapPage(mapAddr);
    if (!physicalAddressOrError) {
      return unexpected<PlatformExpert::DMARange, seL4_Error>(
          seL4_NotEnoughMemory);
    }
    if (startVirt == 0) {
      startVirt = mapAddr;
    }
    if (startPhys == 0) {
      startPhys = physicalAddressOrError.value;
    }
  }
  PlatformExpert::DMARange r;
  r.virt = (void *)startVirt;
  r.phys = startPhys;
  return success<PlatformExpert::DMARange, seL4_Error>(r);
}

void PlatformExpert::releaseDMARange(DMARange &) {}

seL4_Error PlatformExpert::doPowerOff() {
  // QEMU specific! see
  // https://github.com/manu88/Sofa/blob/fce20513a68c2e50a9b5185fc048b4fee0183eac/projects/Sofa/kernel_task/src/Drivers/X86/PCI.c#L40
  // and https://wiki.osdev.org/QEMU_fw_cfg on how to check is running under
  // qemu
  auto stopSlot = issuePortRange(0x604, 0x604 + 1);
  if (!stopSlot) {
    return stopSlot.error;
  }
  return seL4_X86_IOPort_Out16(stopSlot.value, 0x604, 0x2000);
}

bool PlatformExpert::registerBlockDevice(BlockDevice *dev) {
  kprintf("PlatformExpert::registerBlockDevice new block device\n");
  _devices.push_back(dev);
  return false;
}