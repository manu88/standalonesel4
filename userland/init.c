
extern "C"
{
#include "sel4/bootinfo.h"

typedef void (*routine)(void);

void _init(void);

extern routine __preinit_array_start[];
extern routine __preinit_array_end[];
extern routine __init_array_start[];
extern routine __init_array_end[];

void __exec_ctors(void* ptr)
{
    //seL4_InitBootInfo((seL4_BootInfo*)ptr);
    int preinitSize = &__preinit_array_end[0] - &__preinit_array_start[0];

    for (int f = 0; f < preinitSize; f++) {
        __preinit_array_start[f]();
    }

    _init();

    int initSize = &__init_array_end[0] - &__init_array_start[0];
    for (int f = 0; f < initSize; f++) {
        __init_array_start[f]();
    }
}
}