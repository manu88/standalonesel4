#pragma once
#include "sel4.hpp"


class Thread
{
public:
    Thread(seL4_CPtr tcb = 0, seL4_Word entryPoint = 0);


    Thread(const Thread& other):
    _tcb(other._tcb),
    entryPoint(other.entryPoint),
    tcbStackAddr(other.tcbStackAddr),
    endpoint(other.endpoint),
    badge(other.badge)
    {}

    Thread& operator=(const Thread& rhs)
    {
        _tcb = rhs._tcb;
        entryPoint = rhs.entryPoint;
        tcbStackAddr = rhs.tcbStackAddr;
        endpoint = rhs.endpoint;
        badge = rhs.badge;
        return *this;
    }

    seL4_Error resume();

    seL4_CPtr _tcb = 0;
    seL4_Word entryPoint = 0;
    seL4_Word tcbStackAddr = 0;
    seL4_Word endpoint = 0;
    seL4_Word badge = 0;
private:
};