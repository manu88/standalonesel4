#pragma once
#include "sel4.hpp"


class Thread
{
public:
    Thread(seL4_CPtr tcb = 0);


    Thread(const Thread& other):
    _tcb(other._tcb)
    {}

    Thread& operator=(const Thread& rhs)
    {
        _tcb = rhs._tcb;
        return *this;
    }

    seL4_CPtr _tcb;
private:
};