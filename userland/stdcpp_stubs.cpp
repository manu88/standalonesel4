#include "runtime.h"


namespace std
{
    void __throw_bad_function_call()
    {
        assert(0,"std::__throw_bad_function_call was called");
        while(1);
    }; 
}
