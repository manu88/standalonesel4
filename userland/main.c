#include "sel4.hpp"
#include "MemoryManager.hpp"
#include "Hypervisor.hpp"

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
    printf("Hello world :)\n");
    printSel4Config();

    init_tls();
    printf("__sel4_ipc_buffer is at %p\n", (void*) __sel4_ipc_buffer);
    seL4_BootInfo *bi = GetBootInfo();
    __sel4_ipc_buffer = bi->ipcBuffer;
    printf("__sel4_ipc_buffer is at %p\n", (void*) __sel4_ipc_buffer);
    printf("__sel4_print_error is  %u\n", (void*) __sel4_print_error); 
    __sel4_print_error = 1;
    printf("__sel4_print_error is  %u\n", (void*) __sel4_print_error);     
    
    

    MemoryManager memManager;

    memManager.init();
    Hypervisor hyp(memManager);

    hyp.init();

    hyp.eventLoop();

}
} // end extern "C"