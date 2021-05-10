#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"


// Wrapper around seL4_UntypedDesc
struct UntypedRange
{
    bool isDevice;
    size_t size;
    size_t index;
    seL4_Word physAddress;
};

class InitialUntypedPool
{
public:
    InitialUntypedPool();

    void forEachRange(std::function<void(UntypedRange&)>);
private:

};


