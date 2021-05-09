#pragma once
#include <cstddef>

class MemoryManager
{
public:
    MemoryManager();

private:
    size_t _availableNonDeviceUntypedMem {0};
};