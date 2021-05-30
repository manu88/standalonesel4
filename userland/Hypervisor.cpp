#include "Hypervisor.hpp"
#include "sel4.hpp"
#include "InitialUntypedPool.hpp"
#include "Thread.hpp"
#include "runtime.h"


Hypervisor::Hypervisor(MemoryManager & mManager):
_memManager(mManager)
{
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hypervisor");
}


void Hypervisor::init()
{
    printf("Hypervisor: init\n");
/*
    Thread th;
    if(createThread(th) == 0)
    {
        printf("Thread is %s\n ", th.isValid()? "valid":"invalid");
        th.setName("testThread");
        th.resume();
    }
*/
    dumpScheduler();
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

int Hypervisor::createThread(Thread& thread)
{
    printf("Test create thread\n");

    seL4_SlotPos tcbObject = InitialUntypedPool::instance().getSlot(seL4_TCBObject, seL4_TCBBits);
    if((int) tcbObject < 0)
    {
        printf("Hypervisor::createThread: unable to create TCB object\n");
        return -(int) tcbObject;
    }

    seL4_Error err = seL4_TCB_SetSpace(tcbObject, 0, seL4_CapInitThreadCNode, 0, seL4_CapInitThreadVSpace, 0); 
    if(err != seL4_NoError)
    {
		printf("Error: Cannot set TCB space!\n");
        return -err;
    }

    thread = Thread(tcbObject);
    printf("allocated TCB sel is %u\n", tcbObject);
    

    printf("Hypervisor::createThread: retype ok\n");
    return 0;
}