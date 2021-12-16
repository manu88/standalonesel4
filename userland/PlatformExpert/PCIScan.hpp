#pragma once
#include "../sel4.hpp"


#define PCI_CONFIG_ADDRESS (seL4_Word) 0xCF8
#define PCI_CONFIG_DATA (seL4_Word) 0xCFC

class PCIScanner{
public:
    void init(seL4_CPtr pciAddressSlot, seL4_CPtr pciDataSlot);
    void scan();
private:
    seL4_CPtr _pciAddressSlot = 0;
    seL4_CPtr _pciDataSlot = 0;
};