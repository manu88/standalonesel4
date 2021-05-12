#include "sel4.hpp"

extern "C" void _putchar(char c)
{
    seL4_DebugPutChar(c);
}
