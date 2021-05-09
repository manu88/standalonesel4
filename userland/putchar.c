#include "runtime.h"

void _putchar(char c)
{
    seL4_DebugPutChar(c);
}