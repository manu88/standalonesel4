#pragma once
#include "DriverBase.hpp"
#include "Virtio.hpp"

class PCIBlk: public DriverBase{
public:
    bool probe(const PCIDevice& dev) override;
    const char* getName() const noexcept override{
        return "BLK PCI";
    }
    bool addDevice(PlatformExpert & expert,const PCIDevice&) override;

private:
    VirtioDevice _dev;
};
