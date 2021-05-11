

#include "MemoryManager.hpp"
#include "sel4/sel4_arch/syscalls.h"

extern "C"
{

#include "runtime.h"
static int a = 0;


extern unsigned int _tdata_start[];
extern unsigned int _tdata_end[];
extern unsigned int _tbss_end[];

__attribute__ ((constructor)) void foo(void)
{
    a = 10;
}

void start_root()
{
    printf("Hello world a=%i\n", a);
    seL4_DebugNameThread(seL4_CapInitThreadCNode, "test");
    seL4_DebugDumpScheduler();

    printf("IPC buffer is at %p\n",seL4_GetBootInfo()->ipcBuffer);
    seL4_SetIPCBuffer(seL4_GetBootInfo()->ipcBuffer);
    printf("Did set IPCBuffer\n");
    MemoryManager memManager;
    while (1)
    {
        /* code */
    }
}
}