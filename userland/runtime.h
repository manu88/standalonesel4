#pragma once

extern "C" {
#include "printf.h"

#ifndef printf
#define printf(fmt, ...) printf_(fmt, ##__VA_ARGS__)
#endif

static inline void oops() {
  float *f = NULL;
  *f = 42.f;
}

static inline void assert(int pred) {
  if (!pred) {
    oops();
  }
}

#ifndef NOT_REACHED
#define NOT_REACHED() assert(false)
#endif
}
