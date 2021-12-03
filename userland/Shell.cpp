#include "Shell.hpp"
#include "kmalloc.hpp"
#include "runtime.h"
#include <cstring>

int isprint(int c) { return (unsigned)c - 0x20 < 0x5f; }

size_t strlen(const char *s) {
  size_t len = 0;
  for (len = 0; s[len]; (len)++)
    ;
  return len;
}
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

int Shell::newCommand(const string &cmd) {
  if (cmd == "hello") {
    printf("Command is Hello\n");
    return 0;
  }
  printf("Command '%s' does not exist\n", cmd.c_str());
  return -1;
}