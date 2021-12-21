#include "sel4.hpp"
#include "RootServer.hpp"
#include "klog.h"

extern "C"
{
#include "runtime.h"

void init_tls(void);

void printSel4Config(void)
{
    kprintf("---- Sel4 kernel configuration: ----\n");
    kprintf("CONFIG_FSGSBASE_INST : ");
#ifdef CONFIG_FSGSBASE_INST
    kprintf("SET\n");
#else
    kprintf("NOT SET\n");
#endif

    kprintf("CONFIG_SET_TLS_BASE_SELF : ");
#ifdef CONFIG_SET_TLS_BASE_SELF
    kprintf("SET\n");
#else
    kprintf("NOT SET\n");
#endif

kprintf("CONFIG_IOMMU : ");
#ifdef CONFIG_IOMMU
    kprintf("SET\n");
#else
    kprintf("NOT SET\n");
#endif
    kprintf("------------------------------------\n");
}

void start_root()
{
    printSel4Config();

    init_tls();
    seL4_BootInfo *bi = seL4::GetBootInfo();
    __sel4_ipc_buffer = bi->ipcBuffer;
#ifndef ARCH_ARM
    __sel4_print_error = 1;
#endif
    kprintf("Start Root server\n");
    RootServer srv;
    srv.earlyInit();
    srv.lateInit();
    srv.run();

}
} // end extern "C"