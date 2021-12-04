#include "Syscall.hpp"

/*static*/ Expected<Syscall::KMalloc, bool>
Syscall::KMalloc::decodeRequest(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::KMalloc, bool>(false);
  }
  return success<Syscall::KMalloc, bool>(KMalloc(seL4_GetMR(1)));
}
/*static*/ Expected<void *, bool>
Syscall::KMalloc::decodeResponse(const seL4_MessageInfo_t &) {
  return success<void *, bool>((void *)seL4_GetMR(1));
}

/*static*/ Expected<Syscall::KFree, bool>
Syscall::KFree::decodeRequest(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::KFree, bool>(false);
  }
  return success<Syscall::KFree, bool>(KFree((void *)seL4_GetMR(1)));
}
/*static*/ Expected<int, bool>
Syscall::KFree::decodeResponse(const seL4_MessageInfo_t &) {
  return success<int, bool>((int)seL4_GetMR(1));
}

seL4_MessageInfo_t Syscall::perform(const BaseSyscall &b, seL4_Word endpoint) {
  auto numReg = b.getNumMsgRegisters();
  auto info = seL4_MessageInfo_new(0, 0, 0, numReg + 1);
  seL4_SetMR(0, (seL4_Word)b.getId());
  for (size_t i = 0; i < numReg; i++) {
    seL4_SetMR(i + 1, b.getMsgRegister(i));
  }
  if (b.hasResponse()) {
    return seL4_Call(endpoint, info);
  }
  seL4_Send(endpoint, info);
  return seL4_MessageInfo_new(0, 0, 0, 0);
}
