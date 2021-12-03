#pragma once
#include <stddef.h>

class Shell {
public:
  void init();
  void start();
  void onChar(char c);

private:
  enum { BufferSize = 256 };
  char *buffer = nullptr;
  size_t bufferIndex = 0;
};