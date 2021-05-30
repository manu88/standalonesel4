#pragma once
#include <cstddef>
#include "InitialUntypedPool.hpp"


class MemoryManager
{
public:
    MemoryManager();

    void init();

    enum VirtualAddressLayout
    {
        AddressTables = 0x8000000000
    };

private:
    size_t _availableNonDeviceUntypedMem {0};

    
};