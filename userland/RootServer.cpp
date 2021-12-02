#include "RootServer.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer()
{
    printf("Initialize Page Table\n");
    _pt.init(VirtualAddressLayout::AddressTables);
}

void RootServer::run()
{
    printf("RootServer::run\n");

    testPt();
    while (1)
    {
        /* code */
    }   
}

void RootServer::testPt()
{
seL4_Word vaddr = VirtualAddressLayout::ReservedVaddr;

    size_t sizeToTest = 1024;
    for(size_t i=0;i<sizeToTest;i++)
    {
        seL4_Error error = _pt.mapPage(vaddr, seL4_ReadWrite);
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
}