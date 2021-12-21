#pragma once

extern "C" {
#include "printf.h"



static inline void oops() {
  float *f = NULL;
  *f = 42.f;
}

extern void __assert (const char *msg, const char *file, int line);
#define assert(EX) (void)((EX) || (__assert (#EX, __FILE__, __LINE__),0))


#ifndef NOT_REACHED
#define NOT_REACHED() assert(false)
#endif
}
