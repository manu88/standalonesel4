#include "InitialUntypedPool.hpp"
#include "runtime.h"
#include "sel4.hpp"


InitialUntypedPool::UntypedRange::UntypedRange(InitialUntypedPool& pool):
freeOffset(pool._free_offset[index].value)
{

}

InitialUntypedPool::InitialUntypedPool()
{

}

void InitialUntypedPool::forEachNonDeviceRange(std::function<void(InitialUntypedPool::UntypedRange&)> func)
{
    forEachRange([&](UntypedRange&r){
        if(r.isDevice)
        {
            return;
        }
        func(r);
    });
}

void InitialUntypedPool::forEachRange(std::function<void(InitialUntypedPool::UntypedRange&)> func)
{
    const seL4_BootInfo* bootInfo = (const seL4_BootInfo*) seL4_GetBootInfo();
    assert(bootInfo != nullptr, "InitialUntypedPool::forEachRange: null bootinfo");
	for (unsigned sel = bootInfo->untyped.start; sel < bootInfo->untyped.end; sel++) 
    {
        UntypedRange r(*this);
        r.index = sel;
        r.isDevice = bootInfo->untypedList[sel].isDevice;
        r.size = 1UL << bootInfo->untypedList[sel].sizeBits;
        r.physAddress = bootInfo->untypedList[sel].paddr;

        func(r);
    }

}