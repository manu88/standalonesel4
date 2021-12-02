#include "RootServer.hpp"
#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include <sel4/arch/mapping.h> // seL4_MappingFailedLookupLevel

RootServer::RootServer()
{
    printf("Initialize Page Table\n");
    _pt.init(VirtualAddressLayout::AddressTables);
}

void threadMain(seL4_Word p0, seL4_Word p1, seL4_Word p2)
{
    printf("Hello from THREAD\n");
    while (1)
    {
        /* code */
    }
    
}

void RootServer::testThread()
{
    auto tcbOrErr = InitialUntypedPool::instance().allocObject(seL4_TCBObject);
    assert(tcbOrErr);
    _threadTest = Thread(tcbOrErr.value);

    seL4_Word faultEP = 0;
    seL4_Word cspaceRootData = 0;
    seL4_Word vspaceRootData = 0;
    seL4_Error err = seL4_TCB_SetSpace(_threadTest._tcb, faultEP, seL4_CapInitThreadCNode, cspaceRootData, seL4_CapInitThreadVSpace, vspaceRootData);
    assert(err == seL4_NoError);

    seL4_Word tcbStackAddr = currentVirtualAddress;
    auto tcbStackOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbStackOrErr);
    currentVirtualAddress += PAGE_SIZE;

    seL4_Word tlsAddr = currentVirtualAddress;
    auto tcbTlsOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbTlsOrErr);
    currentVirtualAddress += PAGE_SIZE;

    seL4_Word tcbIPC = currentVirtualAddress;
    auto tcbIPCOrErr = _pt.mapPage(currentVirtualAddress, seL4_ReadWrite);
    assert(tcbIPCOrErr);
    currentVirtualAddress += PAGE_SIZE;
    printf("tlsAddr %X  tcbIPC %X\n",tlsAddr, tcbIPC);

    err = seL4_TCB_SetTLSBase(_threadTest._tcb, tlsAddr);
    assert(err == seL4_NoError);

    err = seL4_TCB_SetIPCBuffer(_threadTest._tcb, tcbIPC, tcbIPCOrErr.value);
    assert(err == seL4_NoError);

    err = seL4_TCB_SetPriority(_threadTest._tcb, seL4_CapInitThreadTCB, seL4_MaxPrio);
    assert(err == seL4_NoError);

	seL4_UserContext tcb_context;
	size_t num_regs = sizeof(tcb_context)/sizeof(tcb_context.rax);
	seL4_TCB_ReadRegisters(_threadTest._tcb, 0, 0, num_regs, &tcb_context);

	// pass instruction pointer, stack pointer and arguments in registers
	// according to sysv calling convention
	tcb_context.rip = (seL4_Word)threadMain;  // entry point
	tcb_context.rsp = (seL4_Word)(tcbStackAddr + PAGE_SIZE);  // stack
	tcb_context.rbp = (seL4_Word)(tcbStackAddr + PAGE_SIZE);  // stack
	tcb_context.rdi = (seL4_Word)0; // arg 1: start notification
	tcb_context.rsi = (seL4_Word)0;   // arg 2: vga ram
	tcb_context.rdx = (seL4_Word)0;     // arg 3: ipc endpoint

	printf("rip = 0x%lx, rsp = 0x%lx, rflags = 0x%lx, rdi = 0x%lx, rsi = 0x%lx, rdx = 0x%lx.\n",
		tcb_context.rip, tcb_context.rsp, tcb_context.rflags,
		tcb_context.rdi, tcb_context.rsi, tcb_context.rdx);

	// write registers and start thread
	if(seL4_TCB_WriteRegisters(_threadTest._tcb, 1, 0, num_regs, &tcb_context) != seL4_NoError)
		printf("Error writing TCB registers!\n");

}

void RootServer::run()
{
    printf("RootServer: reserve %zi pages\n", ReservedPages);
    reservePages();
    printf("RootServer: Test paging\n");
    //testPt();

    printf("Test thread\n");
    testThread();

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