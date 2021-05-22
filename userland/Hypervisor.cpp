#include "Hypervisor.hpp"
#include "sel4.hpp"
#include "InitialUntypedPool.hpp"
#include "runtime.h"


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

int Hypervisor::createThread(const char* name)
{
    printf("Test create thread\n");

    seL4_SlotPos tcbObject = InitialUntypedPool::instance().getSlot(seL4_TCBObject, seL4_TCBBits);
    if((int) tcbObject < 0)
    {
        printf("Hypervisor::createThread: unable to create TCB object\n");
        return -1;
    }
    printf("allocated TCB sel is %u\n", tcbObject);
    seL4_DebugNameThread(tcbObject, name);

    printf("Hypervisor::createThread: retype ok\n");
}