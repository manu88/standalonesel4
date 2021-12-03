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
void Shell::start(seL4_Word endpoint) {
  _endpoint = endpoint;
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

int Shell::newCommand(const string &cmd) {
  if (cmd == "sched") {
    seL4_DebugDumpScheduler();
    return 0;
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
      void *ret = kmalloc(size);
      printf("kmalloced at address %lu\n", ret);
      return 0;
    }
    return -1;
  } else if (cmd.starts_with("kfree")) {
    if (cmd.size() < 7) {
      return -1;
    }
    auto args = cmd.substr(6);
    long addr = strtol(args.c_str(), NULL, 10);
    //    printf("Got kfree command args are '%s' addr %zi\n", args.c_str(),
    //    addr);
    if (addr) {
      kfree((void *)addr);
      return 0;
    }
    return -1;
  } else if (cmd == "vm") {
    Syscall::perform(Syscall::Id::VMStats, _endpoint);
    return 0;
  }
  printf("Command '%s' does not exist\n", cmd.c_str());
  return -1;
}