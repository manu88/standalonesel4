#include "kmalloc.hpp"
#include "liballoc.h"
#include "tests.hpp"
#include <assert.h>
#include <cstdlib>
#include <cstring>
//#include "vector.hpp"

void testKmallocator() {
  printf("testKmallocator Base:\n");
  assert(kfree != free);
  size_t memSize = 4096 * 10;
  void *memPtr = malloc(memSize);
  assert(memPtr);
  setMemoryPool(memPtr, memSize, 0);
  assert(getTotalKMallocated() == 0);
  char *ptr = (char *)kmalloc(1024);
  assert(ptr);
  for (int i = 0; i < 1024; i++) {
    ptr[i] = 'c';
  }
  printf("getTotalKMallocated()=%zi\n", getTotalKMallocated());
  assert(getTotalKMallocated() == 1024);
  kfree(ptr);
  assert(getTotalKMallocated() == 0);

  printf("testKmallocator krealloc:\n");
  char *ptr2 = (char *)krealloc(nullptr, 512);
  assert(ptr2);
  strcpy(ptr2, "Hello world");
  printf("Test krealloc\n");
  char *ptr22 = (char *)krealloc(ptr2, 1024);
  printf("Test krealloc OK\n");
  assert(ptr22);
  assert(strcmp(ptr22, "Hello world") == 0);
}