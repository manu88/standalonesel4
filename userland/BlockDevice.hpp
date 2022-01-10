#pragma once
#include <sys/types.h> // ssize_t
#include <stdint.h>

struct BlockDevice {
  virtual ~BlockDevice() {}

  virtual ssize_t read(size_t, uint8_t *, size_t) { return -1; }
};