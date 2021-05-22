#pragma once


class MemoryManager;

class Hypervisor
{
public:

    Hypervisor(MemoryManager & mManager);
    void eventLoop();

    void dumpScheduler();

private:
    /* data */
    MemoryManager &_memManager;

};

