#pragma once

extern "C"
{
    #include "sel4/sel4.h"
}

seL4_BootInfo* GetBootInfo(void);

