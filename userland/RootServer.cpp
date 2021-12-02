#include "RootServer.hpp"
#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer():
_pt(_untypedPool)
{
    printf("Initialize Page Table\n");
    _pt.init(VirtualAddressLayout::AddressTables);
}

void threadMain(seL4_Word tcbEndpointSlot, seL4_Word badge, seL4_Word p2)
{
    printf("Hello from THREAD %X\n", badge);
    auto info = seL4_MessageInfo_new(0, 0, 0, 0);
    seL4_Call(tcbEndpointSlot, info);
    printf("THREAD %X: call returned\n", badge);
    while (1)
    {
        /* code */
    }
}

Thread RootServer::createThread(seL4_Word badge, seL4_Word entryPoint)
{
    printf("1\n");
    auto tcbOrErr = _untypedPool.allocObject(seL4_TCBObject);
    if(!tcbOrErr)
    {
        printf("1 Error %s\n", seL4::errorStr(tcbOrErr.error));
    }
    assert(tcbOrErr);
    auto thread = Thread(tcbOrErr.value, entryPoint);
    thread.badge = badge;
    seL4_Word faultEP = 0;
    seL4_Word cspaceRootData = 0;
    seL4_Word vspaceRootData = 0;
    seL4_Error err = seL4_TCB_SetSpace(thread._tcb, faultEP, seL4_CapInitThreadCNode, cspaceRootData, seL4_CapInitThreadVSpace, vspaceRootData);
    assert(err == seL4_NoError);

    printf("2\n");
    thread.tcbStackAddr = currentVirtualAddress;
    auto tcbStackOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbStackOrErr);
    currentVirtualAddress += PAGE_SIZE;

    printf("3\n");
    seL4_Word tlsAddr = currentVirtualAddress;
    auto tcbTlsOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbTlsOrErr);
    currentVirtualAddress += PAGE_SIZE;

    printf("4\n");
    seL4_Word tcbIPC = currentVirtualAddress;
    auto tcbIPCOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbIPCOrErr);
    currentVirtualAddress += PAGE_SIZE;
    printf("tlsAddr %X  tcbIPC %X\n",tlsAddr, tcbIPC);

    err = seL4_TCB_SetTLSBase(thread._tcb, tlsAddr);
    assert(err == seL4_NoError);

    err = seL4_TCB_SetIPCBuffer(thread._tcb, tcbIPC, tcbIPCOrErr.value);
    assert(err == seL4_NoError);

    err = seL4_TCB_SetPriority(thread._tcb, seL4_CapInitThreadTCB, seL4_MaxPrio);
    assert(err == seL4_NoError);

    auto tcbEndpointSlotOrErr = _untypedPool.getFreeSlot();
    assert(tcbEndpointSlotOrErr);
    seL4_SlotPos tcbEndpointSlot = tcbEndpointSlotOrErr.value;
    err = seL4_CNode_Mint(seL4_CapInitThreadCNode,
                          tcbEndpointSlot,
                          seL4_WordBits,
                          seL4_CapInitThreadCNode,
                          _apiEndpoint,
                          seL4_WordBits,
                          seL4_AllRights,
                          thread.badge);
    assert(err == seL4_NoError);
    thread.endpoint = tcbEndpointSlot;
    return thread;
}

void RootServer::run()
{
    printf("RootServer: reserve %zi pages\n", ReservedPages);
    reservePages();
    //printf("RootServer: Test paging\n");
    //testPt();
    auto apiEpOrErr = _untypedPool.allocObject(seL4_EndpointObject);
    assert(apiEpOrErr);
    _apiEndpoint = apiEpOrErr.value;
    printf("Test thread1\n");
    auto t1 = createThread(42, (seL4_Word)threadMain);
    printf("Test thread2\n");
    auto t2 = createThread(43, (seL4_Word)threadMain);
    printf("Start thread1\n");
    t1.resume();
    printf("Start thread2\n");
    t2.resume();

    while (1)
    {
        seL4_Word sender = 0;
        seL4_MessageInfo_t msgInfo = seL4_Recv(_apiEndpoint, &sender);
        printf("RootTask: Received msg from %X\n", sender);
        seL4_Reply(msgInfo);
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