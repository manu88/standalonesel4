#include "PCIScanner.hpp"
#include "../klog.h"
#include "../runtime.h"
#include "pciDevices.h"

union CommandReg {
  uint16_t value;
  struct Fields {
    uint8_t ioSpace : 1;
    uint8_t memSpace : 1;
    uint8_t busMaster : 1;
    uint8_t specialCycles : 1;
    uint8_t memoryWriteAndInvalidateEnable : 1;
    uint8_t vGAPaletteSnoop : 1;
    uint8_t parityErrorResponse : 1;
    uint8_t reserved0 : 1;
    uint8_t serrEnable : 1;
    uint8_t fastBackToBackEnable : 1;
    uint8_t interruptDisable : 1;
  } fields;
};
static_assert(sizeof(CommandReg) == 2);

union RevIdAndProgIf {
  uint16_t value;
  struct Fields {
    uint8_t revId;
    uint8_t progIf;
  } fields;
};
static_assert(sizeof(RevIdAndProgIf) == 2);

union SubClassAndClass {
  uint16_t value;
  struct Fields {
    uint8_t subclassCode;
    uint8_t classCode;
  } fields;
};
static_assert(sizeof(SubClassAndClass) == 2);

union BistAndHeaderType {
  uint16_t value;
  struct Fields {
    uint8_t headerType;
    uint8_t bist;
  } fields;
};
static_assert(sizeof(BistAndHeaderType) == 2);

static const char *devClassStr(PCIDevice::Class c) {
  switch (c) {
  case PCIDevice::Class::Unclassified:
    return "Unclassified";
  case PCIDevice::Class::MassStorageController:
    return "MassStorageController";
  case PCIDevice::Class::NetworkController:
    return "NetworkController";
  case PCIDevice::Class::DisplayController:
    return "DisplayController";
  case PCIDevice::Class::MultimediaController:
    return "MultimediaController";
  case PCIDevice::Class::MemoryController:
    return "MemoryController";
  case PCIDevice::Class::Bridge:
    return "Bridge";
  case PCIDevice::Class::SimpleCommunicationController:
    return "SimpleCommunicationController";
  case PCIDevice::Class::BaseSystemPeripheral:
    return "BaseSystemPeripheral";
  case PCIDevice::Class::InputDeviceController:
    return "InputDeviceController";
  default:
    return "Unknown class dev";
    break;
  }
}

static void printIOConfig(const PCIDevice &dev, bool compact);

void PCIDevice::print() const {
  kprintf("\n");
  kprintf("bus %X slot %X -> vendor = 0X%X (%s) device = 0X%X ('%s')\n", bus,
          slot, vendorID, vendorName(), deviceID, deviceName());
  kprintf("Header type %X subsystemID 0X%X  subsystem VendorID 0X%X\n",
          headerType, subSystemID, subSystemVendorID);
  kprintf("Class=0x%X (%s) subClass=0X%X progIf=0X%X\n", class_, className(),
          subclassCode, progIf);
  kprintf("IRQ line=%X pin=%X\n", irqLine, irqPin);
  kprintf("Status:");
  kprintf(" interruptStatus: 0X%X\n", status.interruptStatus);
  kprintf(" capabilitiesList: 0X%X\n", status.capabilitiesList);
  kprintf(" is66MHzCapable: 0X%X\n", status.is66MHzCapable);
  kprintf(" fastBackToBackCapable: 0X%X\n", status.fastBackToBackCapable);
  kprintf(" masterDataParityError: 0X%X\n", status.masterDataParityError);
  kprintf(" deVSELTiming: 0X%X\n", status.deVSELTiming);
  kprintf(" signaledTargetAbort: 0X%X\n", status.signaledTargetAbort);
  kprintf(" receivedTargetAbort: 0X%X\n", status.receivedTargetAbort);
  kprintf(" receivedMasterAbort: 0X%X\n", status.receivedMasterAbort);
  kprintf(" signaledSystemError: 0X%X\n", status.signaledSystemError);
  kprintf(" detectedParityError: 0X%X\n", status.detectedParityError);
  kprintf("CMD:\n");

  kprintf(" ioSpace: 0X%X\n", cmd.ioSpace);
  kprintf(" memorySpace: 0X%X\n", cmd.memorySpace);
  kprintf(" busMaster: 0X%X\n", cmd.busMaster);
  kprintf(" specialCycles: 0X%X\n", cmd.specialCycles);
  kprintf(" memoryWriteAndInvalidateEnable: 0X%X\n",
          cmd.memoryWriteAndInvalidateEnable);
  kprintf(" VGAPaletteSnoop: 0X%X\n", cmd.VGAPaletteSnoop);
  kprintf(" parityErrorResponse: 0X%X\n", cmd.parityErrorResponse);
  kprintf(" SERREnable: 0X%X\n", cmd.SERREnable);
  kprintf(" fastBackToBackEnable: 0X%X\n", cmd.fastBackToBackEnable);
  kprintf(" interruptDisable: 0X%X\n", cmd.interruptDisable);

  kprintf("\n");
  printIOConfig(*this, true);
}

const char *PCIDevice::vendorName() const noexcept {
  return libpci_vendorID_str(vendorID);
}
const char *PCIDevice::deviceName() const noexcept {
  return libpci_deviceID_str(vendorID, deviceID);
}
const char *PCIDevice::className() const noexcept {
  return devClassStr(class_);
}

void PCIScanner::init(seL4_CPtr pciAddressSlot, seL4_CPtr pciDataSlot) {
  _pciAddressSlot = pciAddressSlot;
  _pciDataSlot = pciDataSlot;
}

uint16_t PCIScanner::pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                                       uint8_t offset) {
  uint32_t lbus = (uint32_t)bus;
  uint32_t lslot = (uint32_t)slot;
  uint32_t lfunc = (uint32_t)func;
  uint32_t address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) |
                                (offset & 0xFC) | ((uint32_t)0x80000000));

  auto err =
      seL4_X86_IOPort_Out32(_pciAddressSlot, PCI_CONFIG_ADDRESS, address);
  if (err != seL4_NoError) {
    kprintf("error seL4_X86_IOPort_Out8 %i\n", err);
  }
  assert(err == seL4_NoError);

  seL4_X86_IOPort_In32_t val =
      seL4_X86_IOPort_In32(_pciDataSlot, PCI_CONFIG_DATA);
  if (val.error != seL4_NoError) {
    kprintf("Read err=%i, val = 0X%X\n", val.error, val.result);
  }
  assert(val.error == seL4_NoError);
  return (uint16_t)((val.result >> ((offset & 2) * 8)) & 0xFFFF);
}

void PCIScanner::scan() {
  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      uint16_t vendorID = pciConfigReadWord(bus, slot, 0, 0);
      if (vendorID == 0XFFFF) {
        continue;
      }
      uint16_t deviceID = pciConfigReadWord(bus, slot, 0, 0x2);

      union CommandRegisterUnion {
        uint16_t v;
        PCIDevice::CommandRegister cmd;
      };
      CommandRegisterUnion cmd;
      cmd.v = pciConfigReadWord(bus, slot, 0, 0x4);

      union StatusUnion {
        uint16_t v;
        PCIDevice::StatusRegister status;
      };
      StatusUnion sta;
      sta.v = pciConfigReadWord(bus, slot, 0, 0x6);

      RevIdAndProgIf revIdAndProgIf;
      revIdAndProgIf.value = pciConfigReadWord(bus, slot, 0, 0x8);

      SubClassAndClass subClassAndClass;
      subClassAndClass.value = pciConfigReadWord(bus, slot, 0, 0xA);

      BistAndHeaderType bistAndHeaderType;
      bistAndHeaderType.value = pciConfigReadWord(bus, slot, 0, 14);
      auto subSystemVendorID = pciConfigReadWord(bus, slot, 0, 0x2C);
      auto subSystemID = pciConfigReadWord(bus, slot, 0, 0x2E);

      union InterLineAndPin {
        uint16_t v;
        struct F {
          uint8_t line;
          uint8_t pin;
        } fields;
      };
      InterLineAndPin interLineAndPin;
      interLineAndPin.v = pciConfigReadWord(bus, slot, 0, 0X3C);

      PCIDevice dev = {.bus = (uint8_t)bus,
                       .slot = (uint8_t)slot,
                       .fun = 0,
                       .vendorID = vendorID,
                       .deviceID = deviceID,
                       .subclassCode = subClassAndClass.fields.subclassCode,
                       .class_ = static_cast<PCIDevice::Class>(
                           subClassAndClass.fields.classCode),
                       .revId = revIdAndProgIf.fields.revId,
                       .progIf = revIdAndProgIf.fields.progIf,
                       .headerType = bistAndHeaderType.fields.headerType,
                       .subSystemVendorID = subSystemVendorID,
                       .subSystemID = subSystemID,
                       .irqLine = interLineAndPin.fields.line,
                       .irqPin = interLineAndPin.fields.pin,
                       .cfg = {},
                       .status = sta.status,
                       .cmd = cmd.cmd};
      readIOConfig(dev.cfg, bus, slot, 0);
      _devices.push_back(dev);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PCI_BASE_ADDRESS_0 0x10     /* 32 bits */
#define PCI_BASE_ADDRESS_SPACE 0x01 /* 0 = memory, 1 = I/O */
#define PCI_BASE_ADDRESS_SPACE_MEMORY 0x00
#define PCI_BASE_ADDRESS_MEM_TYPE_MASK 0x06
#define PCI_BASE_ADDRESS_MEM_TYPE_32 0x00  /* 32 bit address */
#define PCI_BASE_ADDRESS_MEM_TYPE_1M 0x02  /* Below 1M [obsolete] */
#define PCI_BASE_ADDRESS_MEM_TYPE_64 0x04  /* 64 bit address */
#define PCI_BASE_ADDRESS_MEM_PREFETCH 0x08 /* prefetchable? */
#define PCI_BASE_ADDRESS_MEM_MASK (~0x0fUL)
#define PCI_BASE_ADDRESS_IO_MASK (~0x03UL)
#define PCI_BASE_ADDRESS_SPACE_IO 0x01

/* Get the base address at a given index. Will automatically handle split 64-bit
 * addreses. */
uint64_t PCIDevice::getBaseAddr(int index) const {
  assert(index >= 0 && index < 6);
  if (cfg.base_addr_type[index] != PCI_BASE_ADDRESS_MEM_TYPE_64)
    return (uint64_t)cfg.base_addr[index];
  /* 64-bit mode BARs must have a word after it. */
  assert(index < 5);
  /* And the word before it better be set to 64L mode. */
  assert(cfg.base_addr_64H[index + 1]);
  return ((uint64_t)cfg.base_addr[index]) |
         (((uint64_t)cfg.base_addr[index + 1]) << 32);
}

/* Get the 32-bit base address at given index. Will automatically handle split
 * 64-bit addresses,
 * and cast them (with an assert check that the upper 32-bits are zero). */
uint32_t PCIDevice::getBaseAddr32(int index) const {
  uint64_t baddr = getBaseAddr(index);
  assert((baddr & 0xFFFFFFFFUL) == baddr);
  if ((baddr & 0xFFFFFFFFUL) != baddr) {
    kprintf("WARNING: get_baseaddr32 called for 64-bit address. Address will "
            "be truncated.\n");
    kprintf("         This will most likely lead to problems.\n");
    assert(!"WARNING. Zap this assert to ignore.");
  }
  return (uint32_t)(baddr & 0xFFFFFFFFUL);
}

uint32_t PCIDevice::getBaseAddrSize32(int index) const {
  assert(index >= 0 && index < 6);
  return cfg.base_addr_size[index];
}

/* Print out detailed info about a device's base addresses. */
static void printIOConfig(const PCIDevice &dev, bool compact) {
  for (int i = 0; i < 6; i++) {
    if (compact) {
      /* Display in compact space mode, shoving as much information as possible
       * in a few lines. This is similar to how the Linux kernel PCI debug
       * displays in dmesg. */
      if (dev.cfg.base_addr_64H[i])
        continue;
      if (dev.cfg.base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_IO) {
        kprintf("    BAR%d : [ io 0x%X sz 0x%x szmask 0x%x ]\n", i,
                dev.getBaseAddr(i), dev.cfg.base_addr_size[i],
                dev.cfg.base_addr_size_mask[i]);
      } else {
        kprintf("    BAR%d : [ mem 0x%X sz 0x%x szmask 0x%x %s %s ]\n", i,
                dev.getBaseAddr(i), dev.cfg.base_addr_size[i],
                dev.cfg.base_addr_size_mask[i],
                dev.cfg.base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64
                    ? "64bit"
                    : "",
                dev.cfg.base_addr_prefetchable[i] ? "prefetch" : "");
      }
    } else {
      /* Very verbose and space wasting debug output. */
      kprintf("    BASE_ADDR[%d] ----\n", i);
      if (dev.cfg.base_addr[i] == 0 || dev.cfg.base_addr_64H[i])
        continue;
      kprintf("        base_addr_space[%d]: 0x%x [%s]\n", i,
              dev.cfg.base_addr_space[i],
              dev.cfg.base_addr_space[i] ? "PCI_BASE_ADDRESS_SPACE_IO"
                                         : "PCI_BASE_ADDRESS_SPACE_MEMORY");
      kprintf("        base_addr_type[%d]: 0x%x [ ", i,
              dev.cfg.base_addr_type[i]);
      if (dev.cfg.base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_32)
        kprintf("32bit ");
      if (dev.cfg.base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64)
        kprintf("64bit ");
      if (dev.cfg.base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_1M)
        kprintf("<1M ");
      kprintf("]\n");
      kprintf("        base_addr_prefetchable[%d]: %s\n", i,
              dev.cfg.base_addr_prefetchable[i] ? "yes" : "no");
      kprintf("        base_addr[%d]: 0x%X\n", i, dev.getBaseAddr(i));
      kprintf("        base_addr_size_mask[%d]: 0x%x\n", i,
              dev.cfg.base_addr_size_mask[i]);
    }
  }
}

#define BIT(n) (1ul << (n))
#define CTZ(x) __builtin_ctz(x)

#define MASK_UNSAFE(x) ((BIT(x) - 1ul))

/* The MASK_UNSAFE operation involves using BIT that performs a left shift, this
 * shift is only defined by the C standard if shifting by 1 less than the
 * number of bits in a word. MASK allows both the safe creation of masks, and
 * for creating masks that are larger than what is possible with MASK_UNSAFE, as
 * MASK_UNSAFE cannot create a MASK that is all 1's */
#define MASK(n)                                                                \
  __extension__({                                                              \
    __typeof__(n) _n = (n);                                                    \
    (void)assert((unsigned long)_n <= (sizeof(unsigned long) * 8));            \
    (void)assert(_n > 0);                                                      \
    MASK_UNSAFE(_n - 1) | BIT(_n - 1);                                         \
  })

uint32_t PCIScanner::readReg32(uint8_t bus, uint8_t dev, uint8_t fun,
                               uint8_t reg) {
  reg &= ~MASK(2);
  out32(_pciAddressSlot, PCI_CONFIG_ADDRESS,
        0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
  return in32(_pciDataSlot, PCI_CONFIG_DATA);
}

void PCIScanner::writeReg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg,
                            uint32_t val) {
  reg &= ~MASK(2);
  out32(_pciAddressSlot, PCI_CONFIG_ADDRESS,
        0x80000000 | bus << 16 | dev << 11 | fun << 8 | reg);
  out32(_pciDataSlot, PCI_CONFIG_DATA, val);
}

void PCIScanner::out32(seL4_CPtr slot, uint32_t port_no, uint32_t val) {
  auto err = seL4_X86_IOPort_Out32(slot, port_no, val);
  if (err != seL4_NoError) {
    kprintf("out32Address::error seL4_X86_IOPort_Out8 %i\n", err);
  }
  assert(err == seL4_NoError);
}

uint32_t PCIScanner::in32(seL4_CPtr slot, uint32_t port_no) {
  seL4_X86_IOPort_In32_t val = seL4_X86_IOPort_In32(slot, port_no);
  if (val.error != seL4_NoError) {
    kprintf("in32::Read err=%i, val = 0X%X\n", val.error, val.result);
  }
  assert(val.error == seL4_NoError);
  return val.result;
}

void PCIScanner::readIOConfig(PCIDevice::IOConfig &cfg, uint8_t bus,
                              uint8_t dev, uint8_t fun) {
  for (int i = 0; i < 6; i++) {
    // Read and save the base address assigned by the BIOS.
    uint32_t bios_base_addr =
        readReg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4));
    cfg.base_addr_raw[i] = bios_base_addr;

    if (cfg.base_addr_64H[i]) {
      // Don't bother processing further if this is already part of a 64-bit
      // address.
      cfg.base_addr[i] = cfg.base_addr_raw[i];
      cfg.base_addr_size_mask[i] = 0xFFFFFFFF;
      cfg.base_addr_size[i] = 0;
      kprintf("PCIScanner::readIOConfig %X %X is part of a 64-bit address\n",
              bus, dev);
      continue;
    }

    // Write 0xFFFFFFFF to read the configs.
    writeReg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4), 0xFFFFFFFF);
    uint32_t cfg_base_addr =
        readReg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4));

    if (cfg_base_addr == 0) {
      /* no device here. */
      continue;
    }

    cfg.base_addr_space[i] = cfg_base_addr & PCI_BASE_ADDRESS_SPACE;
    if (cfg.base_addr_space[i] == PCI_BASE_ADDRESS_SPACE_MEMORY) {
      cfg.base_addr_type[i] = (cfg_base_addr & PCI_BASE_ADDRESS_MEM_TYPE_MASK);
      cfg.base_addr_prefetchable[i] =
          (cfg_base_addr & PCI_BASE_ADDRESS_MEM_PREFETCH) > 0;
      cfg.base_addr_size_mask[i] = cfg_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
      if (cfg.base_addr_type[i] == PCI_BASE_ADDRESS_MEM_TYPE_64) {
        /* Handle 64-bit addresses. */
        assert(i < 5);
        // Set up the next BAR entry to be 64H mode.
        cfg.base_addr_64H[i + 1] = true;
        // Set up this BAR entry to be in 64L mode.
        cfg.base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
      } else {
        cfg.base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_MEM_MASK;
      }
    } else { /* PCI_BASE_ADDRESS_SPACE_IO */
      cfg.base_addr[i] = bios_base_addr & PCI_BASE_ADDRESS_IO_MASK;
      cfg.base_addr_size_mask[i] = cfg_base_addr & PCI_BASE_ADDRESS_IO_MASK;
      cfg.base_addr_type[i] = PCI_BASE_ADDRESS_MEM_TYPE_32;
    }

    /* Calculate size from size_mask. */
    cfg.base_addr_size[i] = BIT(CTZ(cfg.base_addr_size_mask[i]));

    // Write back the address set by the BIOS.
    writeReg32(bus, dev, fun, PCI_BASE_ADDRESS_0 + (i * 4), bios_base_addr);
  }
}