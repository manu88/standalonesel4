#include "Shell.hpp"
#include "Syscall.hpp"
#include "kmalloc.hpp"
#include "lib/cstring.h"
#include "runtime.h"
#include "sel4.hpp"
#include <stdlib.h>

void Shell::init() {
  buffer = reinterpret_cast<char *>(kmalloc(BufferSize));
  assert(buffer != nullptr);
  memset(buffer, 0, BufferSize);
}
void Shell::showPrompt() { printf(":>"); }

void Shell::start() {
  printf("RootServer shell. type 'help' for ... well ... help\n");
  showPrompt();
}
void Shell::onChar(char c) {
  if (c == 0XD) {
    buffer[bufferIndex] = 0;
    printf("\n");
    if (strlen(buffer) > 0) {
      auto str = string(buffer, bufferIndex);
      newCommand(str);
    }
    bufferIndex = 0;
    memset(buffer, 0, BufferSize);
    showPrompt();
  } else if (c == 0X7F) { // backspace
    if (bufferIndex > 0) {
      bufferIndex--;
    }
  } else /*if (isprint(c))*/ {
    assert(bufferIndex + 1 < BufferSize);
    buffer[bufferIndex] = c;
    buffer[bufferIndex + 1] = 0;
    printf("%c", c);
    if (++bufferIndex >= BufferSize - 1) {
      bufferIndex = 0;
    }
  }
  /*
    else {
      printf("Other char %X\n", c);
    }
    */
}

int Shell::cmdThread(const string &args){
  if(args == "list"){
    Syscall::perform::thread(Thread::getCurrent()->endpoint, Syscall::ThreadRequest::ThreadOp::List);
  } else if (args.starts_with("suspend")){
    auto argStr = args.substr(8);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(Thread::getCurrent()->endpoint, Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::Suspend, badge));
  } else if (args.starts_with("resume")){
    auto argStr = args.substr(7);
    long badge = strtol(argStr.c_str(), NULL, 10);
    Syscall::perform::thread(Thread::getCurrent()->endpoint, Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::Resume, badge));
  } else if (args.starts_with("prio")){
    auto argStr = args.substr(5);
    char *outArg = nullptr;
    long badge = strtol(argStr.c_str(), &outArg, 10);
    if(outArg == nullptr){
      printf("Missing priority arg\n");
      return -1;
    }
    long prio = strtol(outArg, nullptr, 10);
    Syscall::perform::thread(
      Thread::getCurrent()->endpoint,
      Syscall::ThreadRequest(Syscall::ThreadRequest::ThreadOp::SetPriority,
      badge,
      prio
    ));
  }else{
    printf("Unknown thread command '%s'\n", args.c_str());
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
  } else if (cmd.starts_with("thread")) {
    if(cmd.size() < 8){
      return -1;
    }
    auto args = cmd.substr(7);
    return cmdThread(args);
  } else if (cmd == "help") {
    printf("available commands are help, sched, kmalloc/kfree\n");
    return 0;
  } else if (cmd.starts_with("kmalloc")) {
    if (cmd.size() < 9) {
      return -1;
    }
    auto args = cmd.substr(8);
    long size = strtol(args.c_str(), NULL, 10);
    //    printf("Got kmalloc command args are '%s' size %zi\n", args.c_str(),
    //    size);
    if (size) {
      auto responseOrErr = Syscall::perform::kmalloc(
          Thread::getCurrent()->endpoint, Syscall::KMallocRequest(size));
      if (responseOrErr) {
        printf("kmalloced at address %lu\n", responseOrErr.value.p);
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
      printf("Touching %lu\n", addr);
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
      printf("'%s'\n", cmd.c_str());
    }
    return 0;
  } else if (cmd == "last") {
    printf("Last command returned %i\n", lastRet);
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
        printf("mmapped %zi pages at %lu\n", numPages, retOrErr.value.p);
        return 0;
      }
      return -1;
    }
    return -1;
  }
  printf("Command '%s' does not exist\n", cmd.c_str());
  return -1;
}