#pragma once
#include "../lib/vector.hpp"
#include "../sel4.hpp"
#include <stdint.h>

#define PCI_CONFIG_ADDRESS (seL4_Word)0xCF8
#define PCI_CONFIG_DATA (seL4_Word)0xCFC

struct PCIDevice {
  /* Detailed base address information about a device. */
  struct IOConfig {
    /* PCI_BASE_ADDRESS_MEM address or
      PCI_BASE_ADDRESS_IO address */
    uint32_t base_addr[6];
    /* PCI_BASE_ADDRESS_SPACE_IO or
      PCI_BASE_ADDRESS_SPACE_MEMORY */
    uint8_t base_addr_space[6];
    /* PCI_BASE_ADDRESS_MEM_TYPE_32 or
      PCI_BASE_ADDRESS_MEM_TYPE_64 */
    uint8_t base_addr_type[6];
    /* PCI_BASE_ADDRESS_MEM_PREFETCH */
    uint8_t base_addr_prefetchable[6];
    /* size */
    uint32_t base_addr_size_mask[6];
    uint32_t base_addr_size[6];
    /* raw addr */
    uint32_t base_addr_raw[6];
    /* Is this BAR the higher word of a 64-bit address? If true, then this BAR
      is partial and should not be directly processed in any way. */
    bool base_addr_64H[6];
  };

  struct StatusRegister{
    uint8_t _reserved:2;
    uint8_t interruptStatus:1; // Interrupt Status - Represents the state of the device's INTx# signal. If set to 1 and bit 10 of the Command register (Interrupt Disable bit) is set to 0 the signal will be asserted; otherwise, the signal will be ignored. 
    uint8_t capabilitiesList :1; // Capabilities List - If set to 1 the device implements the pointer for a New Capabilities Linked list at offset 0x34; otherwise, the linked list is not available. 
    uint8_t is66MHzCapable:1; // 66 MHz Capable - If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz. 
    uint8_t _reserved2:1; 
    uint8_t fastBackToBackCapable:1; // Fast Back-to-Back Capable - If set to 1 the device can accept fast back-to-back transactions that are not from the same agent; otherwise, transactions can only be accepted from the same agent. 
    uint8_t masterDataParityError :1; // Master Data Parity Error - This bit is only set when the following conditions are met. The bus agent asserted PERR# on a read or observed an assertion of PERR# on a write, the agent setting the bit acted as the bus master for the operation in which the error occurred, and bit 6 of the Command register (Parity Error Response bit) is set to 1. 
    uint8_t deVSELTiming:1; // DEVSEL Timing - Read only bits that represent the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. Where a value of 0x0 represents fast timing, a value of 0x1 represents medium timing, and a value of 0x2 represents slow timing. 
    uint8_t signaledTargetAbort :1; // Signalled Target Abort - This bit will be set to 1 whenever a target device terminates a transaction with Target-Abort. 
    uint8_t receivedTargetAbort :1; // Received Target Abort - This bit will be set to 1, by a master device, whenever its transaction is terminated with Target-Abort. 
    uint8_t receivedMasterAbort :1; // Received Master Abort - This bit will be set to 1, by a master device, whenever its transaction (except for Special Cycle transactions) is terminated with Master-Abort. 
    uint8_t signaledSystemError :1; // Signalled System Error - This bit will be set to 1 whenever the device asserts SERR#. 
    uint8_t detectedParityError :1; // This bit will be set to 1 whenever the device detects a parity error, even if parity error handling is disabled. 
  };

  enum class Class : uint8_t {
    Unclassified = 0,
    MassStorageController = 1,
    NetworkController = 2,
    DisplayController = 3,
    MultimediaController = 4,
    MemoryController = 5,
    Bridge = 6,
    SimpleCommunicationController = 7,
    BaseSystemPeripheral = 8,
    InputDeviceController = 9
  };
  uint8_t bus;
  uint8_t slot;
  uint8_t fun;
  uint16_t vendorID;
  uint16_t deviceID;
  uint8_t subclassCode;
  Class class_;
  uint8_t revId;
  uint8_t progIf;
  uint8_t headerType;

  uint16_t subSystemVendorID;
  uint16_t subSystemID;

  uint8_t irqLine;
  uint8_t irqPin;

  IOConfig cfg;

  StatusRegister status;

  void print() const;
  const char *vendorName() const noexcept;
  const char *deviceName() const noexcept;
  const char *className() const noexcept;

  uint64_t getBaseAddr(int index) const;
  uint32_t getBaseAddr32(int index) const;

  uint32_t getBaseAddrSize32(int index) const;
};

class PCIScanner {
public:
  void init(seL4_CPtr pciAddressSlot, seL4_CPtr pciDataSlot);
  void scan();

  const vector<PCIDevice> &getDevices() const noexcept { return _devices; }

private:
  uint32_t readReg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg);
  void writeReg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg,
                  uint32_t val);

  void readIOConfig(PCIDevice::IOConfig &cfg, uint8_t bus, uint8_t dev,
                    uint8_t fun);

  void out32(seL4_CPtr slot, uint32_t port_no, uint32_t val);
  uint32_t in32(seL4_CPtr slot, uint32_t port_no);

  uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset);

  seL4_CPtr _pciAddressSlot = 0;
  seL4_CPtr _pciDataSlot = 0;

  vector<PCIDevice> _devices;
};