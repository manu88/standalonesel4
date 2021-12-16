#pragma once
#include "PCIScanner.hpp"

class ObjectFactory;
class PlatformExpert{
public:
    bool init(ObjectFactory* factory);
private:
    PCIScanner _pciScanner;
    ObjectFactory* _factory = nullptr;
};