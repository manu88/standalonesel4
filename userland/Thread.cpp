#include "Thread.hpp"
#include "Platform.hpp"
#include "runtime.h"
#include <cstddef>

Thread::Thread(seL4_CPtr tcb, EntryPoint entryPoint)
    : _tcb(tcb), entryPoint(entryPoint) {}

static void _threadMain(seL4_Word p2) {
  Thread *self = reinterpret_cast<Thread *>(p2);
  self->retValue = self->entryPoint(*self, nullptr);
  printf("Thread %i returned, suspend it\n", self->badge);
  seL4_TCB_Suspend(self->_tcb);
}

seL4_Error Thread::setPriority(seL4_Word prio) {
  seL4_Error err = seL4_TCB_SetPriority(_tcb, seL4_CapInitThreadTCB, prio);
  if (err == seL4_NoError) {
    priority = prio;
  }
  return err;
}

seL4_Error Thread::resume() {
  seL4_UserContext tcb_context;
  size_t num_regs = sizeof(tcb_context) / sizeof(tcb_context.rax);
  seL4_TCB_ReadRegisters(_tcb, 0, 0, num_regs, &tcb_context);

  // pass instruction pointer, stack pointer and arguments in registers
  // according to sysv calling convention
  tcb_context.rip = (seL4_Word)_threadMain;
  tcb_context.rsp = (seL4_Word)(tcbStackAddr + PAGE_SIZE); // stack
  tcb_context.rbp = (seL4_Word)(tcbStackAddr + PAGE_SIZE); // stack
  tcb_context.rdi = (seL4_Word)this;                       // p0
  tcb_context.rsi = (seL4_Word)0;                          // p1
  tcb_context.rdx = (seL4_Word)0;                          // p2
  // write registers and start thread
  return seL4_TCB_WriteRegisters(_tcb, 1, 0, num_regs, &tcb_context);
}