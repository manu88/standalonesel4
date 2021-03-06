#pragma once
#include "sel4.hpp"
#include <functional>

class Thread {
public:
  static bool calledFrom(const Thread &t);
  static bool calledFromMain() { return calledFrom(main); }
  static Thread *getCurrent() { return (Thread *)seL4_GetUserData(); }
  static Thread main;
  using EntryPoint =  std::function<void *(Thread &, void *)>;

  enum State: int {
    Uninitialized = 0,
    Started = 1,
    Running = 2,
    Paused = 3,
    Done = 4
  };
  Thread(seL4_CPtr tcb, EntryPoint entryPoint);

  Thread(const Thread &other)
      : _tcb(other._tcb), entryPoint(other.entryPoint),
        tcbStackAddr(other.tcbStackAddr), endpoint(other.endpoint),
        badge(other.badge), priority(other.priority), _state(other._state) {}

  Thread &operator=(const Thread &rhs) {
    _tcb = rhs._tcb;
    entryPoint = rhs.entryPoint;
    tcbStackAddr = rhs.tcbStackAddr;
    endpoint = rhs.endpoint;
    badge = rhs.badge;
    priority = rhs.priority;
    _state = rhs._state;
    return *this;
  }

  constexpr bool operator==(const Thread &rhs) noexcept {
    return badge == rhs.badge;
  }
  void setName(const char *name);

  bool calledFrom() const noexcept;

  seL4_Error start();
  seL4_Error suspend();
  seL4_Error resume();

  seL4_Word getPriority() const noexcept { return priority; }
  seL4_Error setPriority(seL4_Word prio);

  State getState() const noexcept {
    return _state;
  }
  seL4_CPtr _tcb = 0;
  EntryPoint entryPoint = nullptr;
  seL4_Word tcbStackAddr = 0;
  seL4_Word endpoint = 0;
  seL4_Word badge = 0;
  seL4_Word priority = 0;

  void *retValue = nullptr;

private:
  static void _threadMain(seL4_Word p2);
  State _state;
};
