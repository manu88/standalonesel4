#pragma once


class MemoryManager;

class Hypervisor
{
public:

    Hypervisor(MemoryManager & mManager):
    _memManager(mManager){}

    void eventLoop();


private:
    /* data */
    MemoryManager &_memManager;

};

