#pragma once

extern "C"
{
    #include "sel4/sel4.h"
}

namespace seL4
{
seL4_BootInfo* GetBootInfo(void);
}

