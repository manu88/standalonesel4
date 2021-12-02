#pragma once
#include <functional>
#include <cstddef>
#include "sel4.hpp"
#include "lib/expected.hpp"

#define PAGE_TYPE        seL4_X86_4K
#define PAGE_SIZE        4096

class InitialUntypedPool
{
public:
    using ObjectOrError = Expected<seL4_CPtr, seL4_Error>;

    static auto& instance()
    {
        static InitialUntypedPool _instance;
        return _instance;
    }

    ObjectOrError allocObject(seL4_Word type);

private:
    InitialUntypedPool(){}
};