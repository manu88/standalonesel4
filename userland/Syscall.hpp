#pragma once
#include "lib/expected.hpp"
#include "sel4.hpp"
#include <stddef.h>

namespace Syscall {

enum class ID : seL4_Word { Unknown, VMStats, KMalloc, KFree };

struct BaseSyscall {
  virtual ~BaseSyscall() {}

  virtual size_t getNumMsgRegisters() const noexcept { return 0; }
  virtual seL4_Word getMsgRegister(size_t) const noexcept { return 0; }
  virtual Syscall::ID getId() const noexcept { return Syscall::ID::Unknown; }
  virtual bool hasResponse() const noexcept { return false; }
};

struct VMStats : public BaseSyscall {
  Syscall::ID getId() const noexcept final { return Syscall::ID::VMStats; }
};

struct KMalloc : public BaseSyscall {
  KMalloc(size_t size) : size(size) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final { return size; }
  Syscall::ID getId() const noexcept final { return Syscall::ID::KMalloc; }
  bool hasResponse() const noexcept final { return true; }
  size_t size;

  static Expected<KMalloc, bool> decodeRequest(const seL4_MessageInfo_t &);
  static Expected<void *, bool> decodeResponse(const seL4_MessageInfo_t &);
};

struct KFree : public BaseSyscall {
  KFree(void *ptr) : ptr(ptr) {}
  size_t getNumMsgRegisters() const noexcept final { return 1; }
  seL4_Word getMsgRegister(size_t) const noexcept final {
    return (seL4_Word)ptr;
  }
  Syscall::ID getId() const noexcept final { return Syscall::ID::KFree; }
  bool hasResponse() const noexcept final { return true; }
  void *ptr;

  static Expected<KFree, bool> decodeRequest(const seL4_MessageInfo_t &);
  static Expected<int, bool> decodeResponse(const seL4_MessageInfo_t &);
};

seL4_MessageInfo_t perform(const BaseSyscall &b, seL4_Word endpoint);
}; // namespace Syscall
