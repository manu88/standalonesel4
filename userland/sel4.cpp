#include "sel4.hpp"

//FIXME: use seL4's new way of getting boot info
seL4_BootInfo* GetBootInfo(void)
{
    return seL4_GetBootInfo();
}