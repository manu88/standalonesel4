#include "runtime.h"

namespace std {
void __throw_bad_function_call() {
  printf("std::__throw_bad_function_call was called\n");
  assert(0);
  while (1)
    ;
};

} // namespace std

#ifndef ATEXIT_MAX_FUNCS
#define ATEXIT_MAX_FUNCS 128
#endif

extern "C" {

void __cxa_pure_virtual() { while (1); }

struct atexit_func_entry_t {
  /*
   * Each member is at least 4 bytes large. Such that each entry is 12bytes.
   * 128 * 12 = 1.5KB exact.
   **/
  void (*destructor_func)(void *);
  void *obj_ptr;
  void *dso_handle;
};

atexit_func_entry_t __atexit_funcs[ATEXIT_MAX_FUNCS];
size_t __atexit_func_count = 0;

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso) {

  if (__atexit_func_count >= ATEXIT_MAX_FUNCS) {
    return -1;
  };
  __atexit_funcs[__atexit_func_count].destructor_func = f;
  __atexit_funcs[__atexit_func_count].obj_ptr = objptr;
  __atexit_funcs[__atexit_func_count].dso_handle = dso;
  __atexit_func_count++;
  return 0; /*I would prefer if functions returned 1 on success, but the ABI
               says...*/
}
}