#include "sel4.hpp"
#include "RootServer.hpp"

extern "C"
{
#include "runtime.h"

void init_tls(void);

void printSel4Config(void)
{
    printf("---- Sel4 kernel configuration: ----\n");
    printf("CONFIG_FSGSBASE_INST : ");
#ifdef CONFIG_FSGSBASE_INST
    printf("SET");
#else
    printf("NOT SET");
#endif
    printf("\n");

    printf("CONFIG_SET_TLS_BASE_SELF : ");
#ifdef CONFIG_SET_TLS_BASE_SELF
    printf("SET");
#else
    printf("NOT SET");
#endif
    printf("\n");
    printf("------------------------------------\n");
}


void start_root()
{
    printSel4Config();

    init_tls();
    seL4_BootInfo *bi = seL4::GetBootInfo();
    __sel4_ipc_buffer = bi->ipcBuffer;
//#ifndef ARCH_ARM
    __sel4_print_error = 1;
//#endif
    printf("Start Root server\n");
    RootServer srv;
    srv.earlyInit();
    srv.lateInit();
    srv.run();

}
} // end extern "C"