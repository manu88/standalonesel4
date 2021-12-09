#include "sel4.hpp"

extern "C" void _putchar(char c)
{
//#ifndef ARCH_ARM
    seL4_DebugPutChar(c);
//#endif
}
