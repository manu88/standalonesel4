#pragma once
#include "lib/expected.hpp"
#include "sel4.hpp"
#include <stddef.h>
#include <sys/types.h> // size_t

namespace Syscall {

enum class ID : seL4_Word {
  Unknown,
  Read,
  Debug,
  KMalloc,
  KFree,
  MMap,
  Thread,
  Platform,
  Poweroff
};

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

struct ReadRequest : BaseRequest {
  ReadRequest(size_t sector, size_t size) : sector(sector), size(size) {}
  size_t getNumMsgRegisters() const noexcept final { return 2; }
  seL4_Word getMsgRegister(size_t i) const noexcept final {
    if(i == 0){
      return (seL4_Word)sector;
    }
    return (seL4_Word)size;
  }
  bool hasResponse() const noexcept final { return true; }
  size_t sector;
  size_t size;

  static Expected<ReadRequest, bool> decode(const seL4_MessageInfo_t &);
};

struct ReadResponse : public BaseResponse {
  ReadResponse(ssize_t resp) : resp(resp) {}
  static Expected<ReadResponse, bool> decode(const seL4_MessageInfo_t &);

  ssize_t resp;
};

/* *** *** ***  */

struct ThreadRequest : BaseRequest {
  enum ThreadOp : seL4_Word {
    List,
    Suspend,
    Resume,
    SetPriority,
    StopAndDelete,
    VM,
  };
  ThreadRequest(ThreadOp op, seL4_Word arg1 = 0, seL4_Word arg2 = 0)
      : op(op), arg1(arg1), arg2(arg2) {}
  size_t getNumMsgRegisters() const noexcept final { return 3; }
  seL4_Word getMsgRegister(size_t index) const noexcept final {
    if (index == 0)
      return op;
    if (index == 1)
      return arg1;
    return arg2;
  }
  bool hasResponse() const noexcept final { return true; }
  ThreadOp op;
  seL4_Word arg1 = 0;
  seL4_Word arg2 = 0;
  static Expected<ThreadRequest, bool> decode(const seL4_MessageInfo_t &);
};

/* *** *** ***  */

template <typename RequestType = BaseRequest,
          typename ReturnType = BaseResponse>
Expected<ReturnType, bool> performBase(seL4_Word endpoint, Syscall::ID id,
                                       const RequestType &b = BaseRequest());

namespace perform {

inline Expected<ReadResponse, bool> read(seL4_Word endpoint,
                                               const ReadRequest &r) {
  return performBase<ReadRequest, ReadResponse>(endpoint, ID::Read, r);
}

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

inline Expected<BaseResponse, bool> thread(seL4_Word endpoint,
                                           const ThreadRequest &r) {
  return performBase<ThreadRequest, BaseResponse>(endpoint, ID::Thread, r);
}

inline Expected<BaseResponse, bool>
platform(seL4_Word endpoint, const BaseRequest &r = BaseRequest()) {
  return performBase<BaseRequest, BaseResponse>(endpoint, ID::Platform, r);
}

inline Expected<BaseResponse, bool> poweroff(seL4_Word endpoint, const BaseRequest &r = BaseRequest()) {
  return performBase<BaseRequest, BaseResponse>(endpoint, ID::Poweroff, r);
}


}; // namespace perform
}; // namespace Syscall
