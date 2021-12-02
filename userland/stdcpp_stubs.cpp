#include "runtime.h"

namespace std
{
    void __throw_bad_function_call()
    {
        printf("std::__throw_bad_function_call was called\n");
        assert(0);
        while(1);
    }; 
}
