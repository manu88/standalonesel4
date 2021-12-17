#pragma once
#include "DriverBase.hpp"


class PCIBlk: public DriverBase{
public:
    bool probe(const PCIDevice& dev) override;
    const char* getName() const noexcept override{
        return "BLK PCI";
    }
    bool addDevice(const PCIDevice&, uint32_t) override;
};
