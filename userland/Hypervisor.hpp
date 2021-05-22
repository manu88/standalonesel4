#pragma once
#include "Thread.hpp"


class MemoryManager;


class Hypervisor
{
public:

    Hypervisor(MemoryManager & mManager);

    void init();
    void eventLoop();

    void dumpScheduler();

    int createThread(Thread& thread);


private:
    /* data */
    MemoryManager &_memManager;

};

