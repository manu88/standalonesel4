#pragma once
#include "Thread.hpp"
#include "lib/basic_string.hpp"
#include "lib/vector.hpp"
#include "sel4.hpp"
#include <stddef.h>

class Shell {
public:
  void init();         // called from main thread
  void start();        // called from com1 thread
  void onChar(char c); // called from com1 thread

private:
  int cmdThread(const string &cmd);

  int newCommand(const string &cmd);
  int processNewCommand(const string &cmd);
  void showPrompt();
  enum { BufferSize = 256 };
  char *buffer = nullptr;
  size_t bufferIndex = 0;

  int lastRet = 0;
  vector<string> _history;
};