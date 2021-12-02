#include "Thread.hpp"
#include "Platform.hpp"
#include <cstddef>

Thread::Thread(seL4_CPtr tcb, seL4_Word entryPoint):
_tcb(tcb),
entryPoint(entryPoint)
{}


seL4_Error Thread::resume()
{
	seL4_UserContext tcb_context;
	size_t num_regs = sizeof(tcb_context)/sizeof(tcb_context.rax);
	seL4_TCB_ReadRegisters(_tcb, 0, 0, num_regs, &tcb_context);

	// pass instruction pointer, stack pointer and arguments in registers
	// according to sysv calling convention
	tcb_context.rip = entryPoint;
	tcb_context.rsp = (seL4_Word)(tcbStackAddr + PAGE_SIZE);  // stack
	tcb_context.rbp = (seL4_Word)(tcbStackAddr + PAGE_SIZE);  // stack
	tcb_context.rdi = (seL4_Word)endpoint; // p0
	tcb_context.rsi = (seL4_Word)badge;   // p1
	tcb_context.rdx = (seL4_Word)0;     // p2

	// write registers and start thread
	return seL4_TCB_WriteRegisters(_tcb, 1, 0, num_regs, &tcb_context);
}