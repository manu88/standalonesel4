#pragma once
#include "sel4.hpp"


class TCB
{
public:
    TCB(seL4_CPtr tcb = 0);


    TCB(const TCB& other):
    _tcb(other._tcb)
    {}

    TCB& operator=(const TCB& rhs)
    {
        _tcb = rhs._tcb;
        return *this;
    }

private:
    seL4_CPtr _tcb;
};