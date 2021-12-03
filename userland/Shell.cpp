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

void Shell::start() { printf(":>"); }
void Shell::onChar(char c) {
  if (c == 0XD) {
    buffer[bufferIndex] = 0;
    if (strlen(buffer) > 0) {
      printf("\nEcho '%s'", buffer);
    }
    printf("\n");
    bufferIndex = 0;
    memset(buffer, 0, BufferSize);
    start();
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