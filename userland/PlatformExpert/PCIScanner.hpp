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
      /* Is this BAR the higher word of a 64-bit address? If true, then this BAR is partial
        and should not be directly processed in any way. */
      bool base_addr_64H[6];
  };
  enum class Class: uint8_t{
    Unclassified = 0,
    MassStorageController = 1,
    NetworkController = 2,
    DisplayController  = 3,
    MultimediaController = 4,
    MemoryController  = 5,
    Bridge = 6,
    SimpleCommunicationController = 7,
    BaseSystemPeripheral = 8,
    InputDeviceController = 9 
  };
  uint8_t bus;
  uint8_t slot;
  uint16_t vendorID;
  uint16_t deviceID;
  uint8_t subclassCode;
  Class class_;
  uint8_t revId;
  uint8_t progIf;
  uint8_t headerType;

  uint16_t subSystemVendorID;
  uint16_t subSystemID;

  IOConfig cfg;

  void print() const;
  const char* vendorName()const noexcept;
  const char* deviceName()const noexcept;
  const char* className()const noexcept;

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
  void writeReg32(uint8_t bus, uint8_t dev, uint8_t fun, uint8_t reg, uint32_t val);

  void readIOConfig(PCIDevice::IOConfig &cfg, uint8_t bus, uint8_t dev, uint8_t fun);

  void out32(seL4_CPtr slot, uint32_t port_no, uint32_t val);
  uint32_t in32(seL4_CPtr slot, uint32_t port_no);

  uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func,
                             uint8_t offset);

  seL4_CPtr _pciAddressSlot = 0;
  seL4_CPtr _pciDataSlot = 0;

  vector<PCIDevice> _devices;
};