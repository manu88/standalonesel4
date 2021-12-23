#include "Thread.hpp"
#include "Platform.hpp"
#include "klog.h"
#include "runtime.h"
#include <cstddef>

/*static*/ Thread Thread::main = Thread(seL4_CapInitThreadTCB, nullptr);

Thread::Thread(seL4_CPtr tcb, EntryPoint entryPoint)
    : _tcb(tcb), entryPoint(entryPoint), _state(Thread::State::Uninitialized) {}

/*static*/ void Thread::_threadMain(seL4_Word p2) {
  Thread *self = reinterpret_cast<Thread *>(p2);
  self->_state = Thread::State::Running;
  seL4_SetUserData((seL4_Word)self);
  self->retValue = self->entryPoint(*self, nullptr);
  self->_state = Thread::State::Done;
  kprintf("Thread %i returned, suspend it\n", self->badge);
  self->suspend();
}

seL4_Error Thread::setPriority(seL4_Word prio) {
  seL4_Error err = seL4_TCB_SetPriority(_tcb, seL4_CapInitThreadTCB, prio);
  if (err == seL4_NoError) {
    priority = prio;
  }
  return err;
}

seL4_Error Thread::setIPCBuffer(seL4_Word buffer, seL4_CPtr bufferFrame) {
  return seL4_TCB_SetIPCBuffer(_tcb, buffer, bufferFrame);
}

/*static*/ bool Thread::calledFrom(const Thread &t) {
  return seL4_GetUserData() == (seL4_Word)&t;
}

void Thread::setName(const char *name) { seL4_DebugNameThread(_tcb, name); }

bool Thread::calledFrom() const noexcept {
  return seL4_GetUserData() == (seL4_Word)this;
}

#ifdef ARCH_X86_64
seL4_Error Thread::start() {
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
  auto err = seL4_TCB_WriteRegisters(_tcb, 1, 0, num_regs, &tcb_context);
  if (err == seL4_NoError) {
    _state = Thread::State::Started;
  }
  return err;
}
#elif defined(ARCH_ARM)
seL4_Error Thread::start() { return seL4_IllegalOperation; }
#endif

seL4_Error Thread::suspend() {
  auto err = seL4_TCB_Suspend(_tcb);
  if (err == seL4_NoError) {
    _state = Thread::State::Paused;
  }
  return err;
}

seL4_Error Thread::resume() {
  auto err = seL4_TCB_Resume(_tcb);
  if (err == seL4_NoError) {
    _state = Thread::State::Running;
  }
  return err;
}