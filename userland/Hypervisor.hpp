#pragma once


class MemoryManager;

class Hypervisor
{
public:

    Hypervisor(MemoryManager & mManager);
    void eventLoop();

    void dumpScheduler();

    int createThread(const char* name = "thread");

private:
    /* data */
    MemoryManager &_memManager;

};

