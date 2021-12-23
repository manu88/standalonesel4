#include "Shell.hpp"
#include "Syscall.hpp"
#include "kmalloc.hpp"
#include "lib/cstring.h"
#include "runtime.h"
#include "sel4.hpp"
#include <stdlib.h>
#include "klog.h"
#include "PlatformExpert/Pit.hpp"

void Shell::init() {
  buffer = reinterpret_cast<char*>(kmalloc(BufferSize));
  assert(buffer != nullptr);
  memset(buffer, 0, BufferSize);
}
void Shell::showPrompt() { kprintf(":>"); }

void Shell::start() {
  kprintf("RootServer shell. type 'help' for ... well ... help\n");
  showPrompt();
}
void Shell::onChar(char c) {
  if (c == 0XD) {
    buffer[bufferIndex] = 0;
    kprintf("\n");
    if (strlen(buffer) > 0) {
      auto str = string(buffer, bufferIndex);
      newCommand(str);
    }
    bufferIndex = 0;
    memset(buffer, 0, BufferSize);
    showPrompt();
  } else if (c == 0X7F) { // backspace
    if (bufferIndex > 0) {
      kprintf("\b \b");
      bufferIndex--;
    }
  } else /*if (isprint(c))*/ {
    assert(bufferIndex + 1 < BufferSize);
    buffer[bufferIndex] = c;
    buffer[bufferIndex + 1] = 0;
    kprintf("%c", c);
    if (++bufferIndex >= BufferSize - 1) {
      bufferIndex = 0;
    }
  }
  /*
    else {
      kprintf("Other char %X\n", c);
    }
    */
}

int Shell::cmdThread(const string &args) {
  if (args == "list") {
    Syscall::perform::thread(Thread::getCurrent()->endpoint,
                             Syscall::ThreadRequest::ThreadOp::List);
  } else if (args.starts_with("suspend")) {
    auto argStr = args.substr(8);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(
        Thread::getCurrent()->endpoint,
        Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::Suspend,
                               badge));
  } else if (args.starts_with("vm")) {
    auto argStr = args.substr(3);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(
        Thread::getCurrent()->endpoint,
        Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::VM, badge));
  } else if (args.starts_with("resume")) {
    auto argStr = args.substr(7);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(
        Thread::getCurrent()->endpoint,
        Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::Resume,
                               badge));
  } else if (args.starts_with("del")) {
    auto argStr = args.substr(4);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(
        Thread::getCurrent()->endpoint,
        Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::StopAndDelete,
                               badge));
  } else if (args.starts_with("prio")) {
    auto argStr = args.substr(5);
    char *outArg = nullptr;
    long badge = strtol(argStr.c_str(), &outArg, 10);
    if (outArg == nullptr) {
      kprintf("Missing priority arg\n");
      return -1;
    }
    long prio = strtol(outArg, nullptr, 10);
    Syscall::perform::thread(
        Thread::getCurrent()->endpoint,
        Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::SetPriority,
                               badge, prio));
  } else {
    kprintf("Unknown thread command '%s'\n", args.c_str());
    return -1;
  }
  return 0;
}

int Shell::newCommand(const string &cmd) {
  int r = processNewCommand(cmd);
  lastRet = r;
  if (!_history.empty() && _history.back() == cmd) {
    return r;
  }
  _history.push_back(cmd.c_str());
  return r;
}
int Shell::processNewCommand(const string &cmd) {
  if (cmd == "sched") {
    Syscall::perform::debug(
        Thread::getCurrent()->endpoint,
        Syscall::DebugRequest(Syscall::DebugRequest::Operation::DumpScheduler));
    return 0;
  } else if (cmd.starts_with("read")){
      if (cmd.size() < 6) {
        return -1;
      }
      auto args = cmd.substr(5);
      char *outArg = nullptr;
      size_t sector = strtol(args.c_str(), &outArg, 10);
      if (outArg == nullptr) {
        kprintf("Missing sector arg\n");
        return -1;
      }
      size_t size = strtol(outArg, nullptr, 10);
      auto responseOrErr = Syscall::perform::read(Thread::getCurrent()->endpoint, {sector, size});
      if(!responseOrErr){
        kprintf("Read error %i\n", responseOrErr.error);
        return responseOrErr.error;
      }
      kprintf("Did read %zi bytes\n", responseOrErr.value.resp);
      return responseOrErr.value.resp;
  } else if (cmd.starts_with("sleep")){
    if (cmd.size() < 7) {
      return -1;
    }
    auto args = cmd.substr(6);
    long sec = strtol(args.c_str(), NULL, 10);
    if(sec){
      long ms = sec * MS_IN_S;
      kprintf("Sleep for %i sec -> %i ms\n", sec, ms);
      Syscall::perform::sleep(Thread::getCurrent()->endpoint, ms);
      return 0;
    }
  } else if (cmd.starts_with("thread")) {
    if (cmd.size() < 8) {
      return -1;
    }
    auto args = cmd.substr(7);
    return cmdThread(args);
  } else if (cmd == "help") {
    kprintf("available commands are help, sched, kmalloc/kfree\n");
    return 0;
  } else if (cmd.starts_with("kmalloc")) {
    if (cmd.size() < 9) {
      return -1;
    }
    auto args = cmd.substr(8);
    long size = strtol(args.c_str(), NULL, 10);
    //    kprintf("Got kmalloc command args are '%s' size %zi\n", args.c_str(),
    //    size);
    if (size) {
      auto responseOrErr = Syscall::perform::kmalloc(
          Thread::getCurrent()->endpoint, Syscall::KMallocRequest(size));
      if (responseOrErr) {
        kprintf("kmalloced at address %lu\n", responseOrErr.value.p);
      }
      return 0;
    }
    return -1;
  } else if (cmd.starts_with("touch")) {
    if (cmd.size() < 7) {
      return -1;
    }
    auto args = cmd.substr(6);
    long addr = strtol(args.c_str(), NULL, 10);
    if (addr) {
      kprintf("Touching %lu\n", addr);
      ((char *)addr)[0] = 53;
      return 0;
    }
    return -1;
  } else if (cmd.starts_with("kfree")) {
    if (cmd.size() < 7) {
      return -1;
    }
    auto args = cmd.substr(6);
    long addr = strtol(args.c_str(), NULL, 10);
    if (addr) {
      auto respOrErr = Syscall::perform::kfree(
          Thread::getCurrent()->endpoint, Syscall::KFreeRequest((void *)addr));
      if (respOrErr) {
        return respOrErr.value.response;
      }
      return -1;
    }
    return -1;
  } else if (cmd == "vm") {
    Syscall::perform::debug(
        Thread::getCurrent()->endpoint,
        Syscall::DebugRequest(Syscall::DebugRequest::Operation::VMStats));
    return 0;
  } else if (cmd == "history") {
    for (const auto &cmd : _history) {
      kprintf("'%s'\n", cmd.c_str());
    }
    return 0;
  } else if (cmd == "last") {
    kprintf("Last command returned %i\n", lastRet);
    return lastRet;
  } else if (cmd.starts_with("mmap")) {
    if (cmd.size() < 6) {
      return -1;
    }
    auto args = cmd.substr(5);
    long numPages = strtol(args.c_str(), NULL, 10);
    if (numPages) {
      auto retOrErr = Syscall::perform::mmap(Thread::getCurrent()->endpoint,
                                             Syscall::MMapRequest(numPages));
      if (retOrErr) {
        kprintf("mmapped %zi pages at %lu\n", numPages, retOrErr.value.p);
        return 0;
      }
      return -1;
    }
    return -1;
  } else if (cmd.starts_with("plat")) {
    Syscall::perform::platform(Thread::getCurrent()->endpoint);
    return 0;
  } else if (cmd.starts_with("poweroff")) {
    Syscall::perform::poweroff(Thread::getCurrent()->endpoint);
    return 0;
  }
  kprintf("Command '%s' does not exist\n", cmd.c_str());
  return -1;
}