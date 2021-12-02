#include "Thread.hpp"

Thread::Thread(seL4_CPtr tcb):
_tcb(tcb)
{}