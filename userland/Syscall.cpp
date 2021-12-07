#include "Syscall.hpp"
#include "runtime.h"

/*static*/ Expected<Syscall::DebugRequest, bool>
Syscall::DebugRequest::decode(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::DebugRequest, bool>(false);
  }
  return success<Syscall::DebugRequest, bool>(
      DebugRequest((DebugRequest::Operation)seL4_GetMR(1)));
}

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
      Syscall::KMallocResponse((void *)seL4_GetMR(1)));
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

/*static*/ Expected<Syscall::MMapRequest, bool>
Syscall::MMapRequest::decode(const seL4_MessageInfo_t &info) {
  if (seL4_MessageInfo_get_length(info) < 2) {
    return unexpected<Syscall::MMapRequest, bool>(false);
  }
  return success<Syscall::MMapRequest, bool>(MMapRequest(seL4_GetMR(1)));
}

/*static*/ Expected<Syscall::MMapResponse, bool>
Syscall::MMapResponse::decode(const seL4_MessageInfo_t &) {
  return success<Syscall::MMapResponse, bool>(
      Syscall::MMapResponse((void *)seL4_GetMR(1)));
}

Expected<Syscall::ThreadRequest, bool> Syscall::ThreadRequest::decode(const seL4_MessageInfo_t &){
    return success<Syscall::ThreadRequest, bool>(
      Syscall::ThreadRequest(
        (Syscall::ThreadRequest::ThreadOp)seL4_GetMR(1),
        seL4_GetMR(2),
        seL4_GetMR(3)
        ));
}

template <typename RequestType, typename ReturnType>
Expected<ReturnType, bool>
Syscall::performBase(seL4_Word endpoint, Syscall::ID id, const RequestType &b) {
  auto numReg = b.getNumMsgRegisters();
  auto info = seL4_MessageInfo_new(0, 0, 0, numReg + 1);
  seL4_SetMR(0, (seL4_Word)id);
  for (size_t i = 0; i < numReg; i++) {
    auto v = b.getMsgRegister(i);
    seL4_SetMR(i + 1, v);
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

template Expected<Syscall::MMapResponse, bool>
Syscall::performBase<Syscall::MMapRequest>(seL4_Word endpoint, Syscall::ID id,
                                           const Syscall::MMapRequest &b);

template Expected<Syscall::KFreeResponse, bool>
Syscall::performBase<Syscall::KFreeRequest>(seL4_Word endpoint, Syscall::ID id,
                                            const Syscall::KFreeRequest &b);

template Expected<Syscall::BaseResponse, bool>
Syscall::performBase<Syscall::ThreadRequest>(seL4_Word endpoint, Syscall::ID id,
                                            const Syscall::ThreadRequest &b);
