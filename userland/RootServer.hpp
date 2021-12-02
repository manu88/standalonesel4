#pragma once
#include "PageTable.hpp"
#include "Thread.hpp"

class RootServer
{
public:
    RootServer();
    void run();
private:
    enum{ReservedPages = 10};
    enum VirtualAddressLayout // Layout of root server, not other processes!!
    {
        AddressTables = 0x8000000000,
        ReservedVaddr = 0x8000001000, // size is ReservedPages pages
    };
    seL4_Word currentVirtualAddress = ReservedVaddr + (ReservedPages*PAGE_SIZE);

    void reservePages();
    void testPt();
    void testThread();
    PageTable _pt;

    Thread _threadTest;
};