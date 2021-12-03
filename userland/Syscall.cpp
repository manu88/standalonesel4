#include "Syscall.hpp"

struct SyscallDetail {
  seL4_Uint64 length = 1;
  bool isCall = false;
};

static SyscallDetail syscalls[] = {
    {.length = 1}, // Unknown
    {.length = 1}  // VMStats
};

void Syscall::perform(Syscall::Id syscall, seL4_Word endpoint) {
  auto detail = syscalls[(int)syscall];
  auto info = seL4_MessageInfo_new(0, 0, 0, detail.length);
  seL4_SetMR(0, (seL4_Word)syscall);
  seL4_Send(endpoint, info);
}