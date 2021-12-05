#include "Syscall.hpp"

/*static*/ Expected<Syscall::KMallocRequest, bool>
Syscall::KMallocRequest::decode(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::KMallocRequest, bool>(false);
  }
  return success<Syscall::KMallocRequest, bool>(KMallocRequest(seL4_GetMR(1)));
}

/*static*/ Expected<Syscall::KMallocResponse, bool>
Syscall::KMallocResponse::decode(const seL4_MessageInfo_t &) {
  return success<Syscall::KMallocResponse, bool>(
      Syscall::KMallocResponse(seL4_GetMR(1)));
}

/*static*/ Expected<Syscall::KFreeRequest, bool>
Syscall::KFreeRequest::decode(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::KFreeRequest, bool>(false);
  }
  return success<Syscall::KFreeRequest, bool>(
      KFreeRequest((void *)seL4_GetMR(1)));
}

/*static*/ Expected<Syscall::KFreeResponse, bool>
Syscall::KFreeResponse::decode(const seL4_MessageInfo_t &) {
  return success<Syscall::KFreeResponse, bool>(
      Syscall::KFreeResponse(seL4_GetMR(1)));
}

template <typename RequestType, typename ReturnType>
Expected<ReturnType, bool>
Syscall::performBase(seL4_Word endpoint, Syscall::ID id, const RequestType &b) {
  auto numReg = b.getNumMsgRegisters();
  auto info = seL4_MessageInfo_new(0, 0, 0, numReg + 1);
  seL4_SetMR(0, (seL4_Word)id);
  for (size_t i = 0; i < numReg; i++) {
    seL4_SetMR(i + 1, b.getMsgRegister(i));
  }
  if (b.hasResponse()) {
    auto retInfo = seL4_Call(endpoint, info);
    return ReturnType::decode(retInfo);
    // return success<ReturnType, bool>(ReturnType(retInfo));
  }
  seL4_Send(endpoint, info);

  return success<ReturnType, bool>(ReturnType(0));
}

template Expected<Syscall::BaseResponse, bool>
Syscall::performBase(seL4_Word endpoint, Syscall::ID id, const BaseRequest &b);

template Expected<Syscall::KMallocResponse, bool>
Syscall::performBase<Syscall::KMallocRequest>(seL4_Word endpoint,
                                              Syscall::ID id,
                                              const Syscall::KMallocRequest &b);

template Expected<Syscall::KFreeResponse, bool>
Syscall::performBase<Syscall::KFreeRequest>(seL4_Word endpoint, Syscall::ID id,
                                            const Syscall::KFreeRequest &b);
