#pragma once
#include "PageTable.hpp"


class RootServer
{
public:
    RootServer();
    void run();
private:

    enum VirtualAddressLayout // Layout of root server, not other processes!!
    {
        AddressTables = 0x8000000000,
        ReservedVaddr = 0x8000001000, // size is ReservedPages pages
    };

    enum{ReservedPages = 10};
    void reservePages();
    void testPt();
    PageTable _pt;
};