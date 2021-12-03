#pragma once
#include "lib/basic_string.hpp"
#include "sel4.hpp"
#include <stddef.h>

class Shell {
public:
  void init();                    // called from main thread
  void start(seL4_Word endpoint); // called from com1 thread
  void onChar(char c);            // called from com1 thread

private:
  int newCommand(const string &cmd);
  void showPrompt();
  enum { BufferSize = 256 };
  char *buffer = nullptr;
  size_t bufferIndex = 0;

  seL4_Word _endpoint = 0;
};