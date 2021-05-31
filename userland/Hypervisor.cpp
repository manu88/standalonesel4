#include "Hypervisor.hpp"
#include "sel4.hpp"
#include "InitialUntypedPool.hpp"
#include "MemoryManager.hpp"
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

    seL4_Word vaddr = MemoryManager::VirtualAddressLayout::AddressTables;

    size_t sizeToTest = 256 * 512;
    for(size_t i=0;i<sizeToTest;i++)
    {
        seL4_Error error = _memManager.mapPage(vaddr, seL4_ReadWrite);
        if (error == seL4_FailedLookup) {
            printf("Missing intermediate paging structure at level %lu\n", seL4_MappingFailedLookupLevel());
        }
        else if (error != seL4_NoError)
        {
            printf("Test mapping error = %i at %i\n", error, i);
        }
        auto test = reinterpret_cast<size_t*>(vaddr);
        *test = i;
        assert(*test == i, "");
        vaddr += 4096;
        if(i%100 == 0)
        {
            printf("Page %zi/%zi ok\n", i, sizeToTest);
        }
    }
    
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

    seL4_SlotPos tcbObject = InitialUntypedPool::instance().allocObject(seL4_TCBObject);
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