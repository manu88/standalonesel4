#pragma once
#include "sel4.hpp"
#include <functional>

class Thread {
public:
  static bool calledFrom(const Thread &t);
  static bool calledFromMain() { return calledFrom(main); }
  static Thread main;
  using EntryPoint = std::function<void *(Thread &, void *)>;

  Thread(seL4_CPtr tcb = 0, EntryPoint entryPoint = 0);

  Thread(const Thread &other)
      : _tcb(other._tcb), entryPoint(other.entryPoint),
        tcbStackAddr(other.tcbStackAddr), endpoint(other.endpoint),
        badge(other.badge) {}

  Thread &operator=(const Thread &rhs) {
    _tcb = rhs._tcb;
    entryPoint = rhs.entryPoint;
    tcbStackAddr = rhs.tcbStackAddr;
    endpoint = rhs.endpoint;
    badge = rhs.badge;
    return *this;
  }

  bool calledFrom() const noexcept;

  seL4_Error resume();

  seL4_Word getPriority() const noexcept { return priority; }
  seL4_Error setPriority(seL4_Word prio);

  seL4_CPtr _tcb = 0;
  EntryPoint entryPoint = 0;
  seL4_Word tcbStackAddr = 0;
  seL4_Word endpoint = 0;
  seL4_Word badge = 0;
  seL4_Word priority = 0;

  void *retValue = nullptr;

private:
};