#pragma once
#include <stdint.h>

class PCIDevice;

class DriverBase{
public:
    virtual ~DriverBase(){}
    virtual bool probe(const PCIDevice& ) {
        return false;
    }
    virtual const char* getName() const noexcept{
        return nullptr;
    }
    virtual bool addDevice(const PCIDevice&, uint32_t){return false;}
};
