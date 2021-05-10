#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"


/*
Heavily inspired from Genode:
https://github.com/genodelabs/genode/blob/master/repos/base-sel4/
*/
class InitialUntypedPool
{
public:
    // Wrapper around seL4_UntypedDesc
    struct UntypedRange
    {
        UntypedRange(InitialUntypedPool&);
        bool isDevice;
        size_t size;
        size_t index;
        addr_t physAddress;

        /* offset to the unused part of the untyped memory range */
        addr_t &freeOffset;

    };
    InitialUntypedPool();

    void forEachRange(std::function<void(UntypedRange&)>);
    void forEachNonDeviceRange(std::function<void(UntypedRange&)>);
private:

    enum { MAX_UNTYPED = (size_t)CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS };

    struct Free_offset { addr_t value = 0; };

    Free_offset _free_offset[MAX_UNTYPED];

};


