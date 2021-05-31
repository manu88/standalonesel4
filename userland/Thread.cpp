#include "Thread.hpp"

Thread::Thread():
_invalid(true),
_tcbObject(0)
{}

Thread::Thread(seL4_SlotPos tcbObject):
_invalid(false),
_tcbObject(tcbObject)
{}

void Thread::setName(const char* name)
{
    if(isValid())
    {
        seL4_DebugNameThread(_tcbObject, name);
    }
}

int Thread::setPriority(seL4_Word prio)
{
    if(!isValid())
    {
        return seL4_InvalidCapability;
    }

    seL4_Error err = seL4_TCB_SetPriority(_tcbObject, seL4_CapInitThreadTCB, (seL4_Word) prio);
    if(err == seL4_NoError)
    {
        _prio = prio;
    }
    return err;
}

int Thread::resume()
{
    return -1;
}