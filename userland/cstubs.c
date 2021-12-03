#include "runtime.h"
#include "sel4.hpp"

int __errno_location = 0;
// TODO: actual implementation :)
extern "C" int __cxa_guard_acquire(int *)
{
    return 0;
}
extern "C" void __cxa_guard_release(int*)
{

}
extern "C" void __stack_chk_fail(void)
{
    
}


extern "C" void __assert_fail( char const* assertion,  char const* file, int line,  char const* function)
{
    printf("assertion failed %s file line %d in function %s %s\n", assertion, file, line, function);
    oops();
}


extern "C" char *strcpy(char *dest,  char const*src)
{
    char *save = dest;
    while((*dest++ = *src++));
    return save;
}
