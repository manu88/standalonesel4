#include "runtime.h"


namespace std
{
    void __throw_bad_function_call()
    {
        assert(0,"std::__throw_bad_function_call was called");
        while(1);
    }; 
}

extern "C"
{
    // TODO: actual implementation :)
    int __cxa_guard_acquire(int *)
    {
        return 0;
    }
    void __cxa_guard_release(int*)
    {

    }
}