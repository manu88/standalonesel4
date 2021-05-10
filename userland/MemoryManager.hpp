#pragma once
#include <cstddef>
#include "UntypedRange.hpp"

class MemoryManager
{
public:
    MemoryManager();

private:
    size_t _availableNonDeviceUntypedMem {0};

    InitialUntypedPool _untypeds;
};