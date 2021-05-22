#include "Hypervisor.hpp"
#include "runtime.h"
#include "sel4.hpp"


Hypervisor::Hypervisor(MemoryManager & mManager):
_memManager(mManager)
{
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hypervisor");
}

void Hypervisor::eventLoop()
{
    printf("Hypervisor: start event loop\n");
    

    while (1)
    {}
}

void Hypervisor::dumpScheduler()
{
    seL4_DebugDumpScheduler();
}