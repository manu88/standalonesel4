#pragma once
extern "C"
{ 
#include "sel4/sel4.h"

//FIXME: use seL4's new way of getting boot info
static inline seL4_BootInfo* GetBootInfo(void)
{
    return seL4_GetBootInfo();
}

typedef seL4_Word addr_t;
}