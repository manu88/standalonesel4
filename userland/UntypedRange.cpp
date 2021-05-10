#include "UntypedRange.hpp"
#include "runtime.h"
#include "sel4.hpp"


InitialUntypedPool::InitialUntypedPool()
{

}

void InitialUntypedPool::forEachRange(std::function<void(UntypedRange&)> func)
{
    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo();
    assert(bootInfo != nullptr, "InitialUntypedPool::forEachRange: null bootinfo");
	for (unsigned sel = bootInfo->untyped.start; sel < bootInfo->untyped.end; sel++) 
    {
        UntypedRange r;
        r.index = sel;
        r.isDevice = bootInfo->untypedList[sel].isDevice;
        r.size = 1UL << bootInfo->untypedList[sel].sizeBits;
        r.physAddress = bootInfo->untypedList[sel].paddr;

        func(r);
    }

}