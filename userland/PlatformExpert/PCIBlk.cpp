#include "PCIBlk.hpp"
#include "PCIScanner.hpp"

bool PCIBlk::probe(const PCIDevice& dev){
    if(dev.class_ == PCIDevice::Class::MassStorageController){
        return true;
    }
    return false;
}

bool PCIBlk::addDevice(const PCIDevice&, uint32_t){
    return false;
} 