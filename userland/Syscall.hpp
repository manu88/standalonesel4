#pragma once
#include "lib/expected.hpp"
#include "sel4.hpp"
#include <stddef.h>

namespace Syscall {

enum class ID : seL4_Word { Unknown, Debug, KMalloc, KFree, MMap };

struct BaseRequest {
  virtual ~BaseRequest() {}
  virtual size_t getNumMsgRegisters() const noexcept { return 0; }
  virtual seL4_Word getMsgRegister(size_t) const noexcept { return 0; }
  virtual bool hasResponse() const noexcept { return false; }
};

struct BaseResponse {
  BaseResponse() {}
  BaseResponse(int) {}
  virtual ~BaseResponse() {}

  static Expected<BaseResponse, bool> decode(const seL4_MessageInfo_t &) {
    return success<BaseResponse, bool>(BaseResponse());
  }
};

/* *** *** ***  */

struct DebugRequest : BaseRequest {
  enum Operation : seL4_Word { Unknown, VMStats, DumpScheduler };
  DebugRequest(int) : op(Operation::Unknown) {}
  explicit DebugRequest(Operation op) : op(op) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final {
    return (seL4_Word)op;
  }
  Operation op;

  static Expected<DebugRequest, bool> decode(const seL4_MessageInfo_t &);
};

/* *** *** ***  */

struct KMallocRequest : BaseRequest {
  KMallocRequest(size_t s) : size(s) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final { return size; }
  bool hasResponse() const noexcept final { return true; }
  size_t size;

  static Expected<KMallocRequest, bool> decode(const seL4_MessageInfo_t &);
};

struct KMallocResponse : public BaseResponse {
  KMallocResponse(void *p) : p(p) {}
  static Expected<KMallocResponse, bool> decode(const seL4_MessageInfo_t &);

  void *p = nullptr;
};

/* *** *** ***  */

struct KFreeRequest : BaseRequest {
  KFreeRequest(void *ptr) : ptr(ptr) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final {
    return (seL4_Word)ptr;
  }
  bool hasResponse() const noexcept final { return true; }
  void *ptr;

  static Expected<KFreeRequest, bool> decode(const seL4_MessageInfo_t &);
};

struct KFreeResponse : public BaseResponse {
  KFreeResponse(int) {}
  static Expected<KFreeResponse, bool> decode(const seL4_MessageInfo_t &);

  int response = 0;
};

/* *** *** ***  */

struct MMapRequest : BaseRequest {
  MMapRequest(size_t numPages) : numPages(numPages) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final {
    return (seL4_Word)numPages;
  }
  bool hasResponse() const noexcept final { return true; }
  size_t numPages;

  static Expected<MMapRequest, bool> decode(const seL4_MessageInfo_t &);
};

struct MMapResponse : public BaseResponse {
  MMapResponse(void *p) : p(p) {}
  static Expected<MMapResponse, bool> decode(const seL4_MessageInfo_t &);

  void *p = nullptr;
};

/* *** *** ***  */

template <typename RequestType = BaseRequest,
          typename ReturnType = BaseResponse>
Expected<ReturnType, bool> performBase(seL4_Word endpoint, Syscall::ID id,
                                       const RequestType &b = BaseRequest());

namespace perform {
inline Expected<KMallocResponse, bool> kmalloc(seL4_Word endpoint,
                                               const KMallocRequest &r) {
  return performBase<KMallocRequest, KMallocResponse>(endpoint, ID::KMalloc, r);
}

inline Expected<KFreeResponse, bool> kfree(seL4_Word endpoint,
                                           const KFreeRequest &r) {
  return performBase<KFreeRequest, KFreeResponse>(endpoint, ID::KFree, r);
}

inline Expected<BaseResponse, bool> debug(seL4_Word endpoint,
                                          const DebugRequest &r) {
  return performBase<BaseRequest, BaseResponse>(endpoint, ID::Debug, r);
}

inline Expected<MMapResponse, bool> mmap(seL4_Word endpoint,
                                         const MMapRequest &r) {
  return performBase<MMapRequest, MMapResponse>(endpoint, ID::MMap, r);
}

}; // namespace perform
}; // namespace Syscall
