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
    printf("RootServer: reserve %zi pages\n", ReservedPages);
    reservePages();
    printf("RootServer: Test paging\n");
    //testPt();

    
    while (1)
    {
        /* code */
    }   
}

void RootServer::reservePages()
{
    seL4_Word vaddr = VirtualAddressLayout::ReservedVaddr;
    for(int i=0;i<ReservedPages;i++)
    {
        auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
        assert(capOrError);
        vaddr += PAGE_SIZE;
    }

    printf("Test reserved pages\n");
    vaddr = VirtualAddressLayout::ReservedVaddr;
    for(int i=0;i<ReservedPages;i++)
    {
        memset((void*) vaddr, 0, PAGE_SIZE);
        vaddr += PAGE_SIZE;
    }
    vaddr = VirtualAddressLayout::ReservedVaddr;
    for(int i=0;i<ReservedPages;i++)
    {
        for(int j=0;j<PAGE_SIZE;j++)
        {
            assert(reinterpret_cast<char*>(vaddr)[j] == 0);
            (reinterpret_cast<char*>(vaddr))[j] = 0;
        }
    }    
}

void RootServer::testPt()
{
    seL4_Word vaddr = VirtualAddressLayout::ReservedVaddr + (ReservedPages*PAGE_SIZE);

    size_t sizeToTest = 1024;
    for(size_t i=0;i<sizeToTest;i++)
    {
        auto capOrError = _pt.mapPage(vaddr, seL4_ReadWrite);
        if (capOrError.error == seL4_FailedLookup) {
            printf("Missing intermediate paging structure at level %lu\n", seL4_MappingFailedLookupLevel());
        }
        else if (capOrError.error != seL4_NoError)
        {
            printf("Test mapping error = %i at %i\n", capOrError.error, i);
        }
        auto test = reinterpret_cast<size_t*>(vaddr);
        *test = i;
        assert(*test == i);
        vaddr += 4096;
        if(i%100 == 0)
        {
            printf("Page %zi/%zi ok\n", i, sizeToTest);
        }
    }

    printf("After test, vaddr is at %X\n", vaddr);
    vaddr = VirtualAddressLayout::ReservedVaddr + (ReservedPages*PAGE_SIZE);
    for(size_t i=0;i<sizeToTest;i++)
    {
        auto test = reinterpret_cast<size_t*>(vaddr);
//        *test = i;
        assert(*test == i);
        vaddr += 4096;
    }
    printf("After test OK \n");
}