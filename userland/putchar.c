#include "runtime.h"
#include "sel4/sel4_arch/syscalls.h"
void _putchar(char c)
{
    seL4_DebugPutChar(c);
}