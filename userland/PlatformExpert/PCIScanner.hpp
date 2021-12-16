#pragma once
#include "../sel4.hpp"
#include "../lib/vector.hpp"
#include <stdint.h>

#define PCI_CONFIG_ADDRESS (seL4_Word) 0xCF8
#define PCI_CONFIG_DATA (seL4_Word) 0xCFC

struct PCIDevice{
    uint8_t bus;
    uint8_t slot;
    uint16_t vendorID;
    uint16_t deviceID;
    uint8_t subclassCode;
    uint8_t classCode;

    void print() const;
};

class PCIScanner{
public:
    void init(seL4_CPtr pciAddressSlot, seL4_CPtr pciDataSlot);
    void scan();

    const vector<PCIDevice>& getDevices() const noexcept{
        return _devices;
    }
private:
    uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

    seL4_CPtr _pciAddressSlot = 0;
    seL4_CPtr _pciDataSlot = 0;

    vector<PCIDevice> _devices;
};