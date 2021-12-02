#pragma once
#include <cstddef>
#include "InitialUntypedPool.hpp"


struct Page
{
    seL4_CPtr frame;
};

class PageTable
{
public:
    void init(seL4_Word vaddr);

    seL4_Error mapPage(seL4_Word vaddr, seL4_CapRights_t rights);
    seL4_Error unmapPage();

private:
    seL4_CPtr pdpt = 0;
    seL4_CPtr pd = 0;
    seL4_CPtr pt = 0;
};