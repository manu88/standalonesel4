#pragma once
#include "PageTable.hpp"


class RootServer
{
public:
    RootServer();
    void run();
private:

    enum VirtualAddressLayout
    {
        AddressTables = 0x8000000000,
        ReservedVaddr = 0x8000001000,
    };

    enum{ReservedPages = 10};
    void testPt();
    PageTable _pt;
};