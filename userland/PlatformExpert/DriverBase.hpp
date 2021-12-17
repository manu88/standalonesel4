#pragma once
#include <stdint.h>

class PCIDevice;
class PlatformExpert;

class DriverBase{
public:
    virtual ~DriverBase(){}
    virtual bool probe(const PCIDevice& ) {
        return false;
    }
    virtual const char* getName() const noexcept{
        return nullptr;
    }
    virtual bool addDevice(PlatformExpert &, const PCIDevice&){return false;}
};
