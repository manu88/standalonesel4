#include "sel4.hpp"
#include "runtime.h"
extern "C"
{

static int a = 0;


__attribute__ ((constructor)) void foo(void)
{
    a = 10;
}

extern "C" char *strcpy(char *dest,  char const*src)
{
    char *save = dest;
    while((*dest++ = *src++));
    return save;
}
extern "C" void __assert_fail( char const* assertion,  char const* file, int line,  char const* function)
{
}

extern "C" void __stack_chk_fail(void)
{
    
}


void printSel4Config()
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
}

void start_root()
{
    printf("Hello world :)\n");
    printSel4Config();
//  printf("__sel4_ipc_buffer is at %p\n", (void*) __sel4_ipc_buffer);
//  seL4_DebugNameThread(seL4_CapInitThreadTCB, "foo");
//  seL4_DebugDumpScheduler();
    while (1)
    {}
}
}