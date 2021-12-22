#include "kmalloc.hpp"
#include "lib/cstring.h"
#ifdef UNIT_TESTS
#include "sel4_mock.hpp"
#include <cassert>
#else
#include "mutex.h"
#include "runtime.h"
#endif
#include <stddef.h>
#include "klog.h"
#include "Platform.hpp"
#include "liballoc.h"
#include "klog.h"
extern "C" {

static void *startMemPool = nullptr;
static size_t sizeMemPool = 0;
static size_t indexInMemPool = 0;
static int hasMemoryPool = 0;
static seL4_CPtr _mutexNotif = 0;
static sync_mutex_t _lock = {};

void setMemoryPool(void *start, size_t size, seL4_CPtr mutexNotif) {
  kprintf("setMemoryPool at %p size=%zi\n", start, size);
  startMemPool = start;
  sizeMemPool = size;
  _mutexNotif = mutexNotif;
  hasMemoryPool = 1;

  if (sync_mutex_init(&_lock, _mutexNotif) != 0) {
    assert(0);
  }
}

int liballoc_lock(){
  assert(hasMemoryPool);
  sync_mutex_lock(&_lock);
  return 0;
}

int liballoc_unlock(){
  assert(hasMemoryPool);
  sync_mutex_unlock(&_lock);
  return 0;
}

void* liballoc_alloc(size_t numPages){
  assert(hasMemoryPool);
  kprintf("liballoc_alloc for %zi pages, current index is at %zi\n", numPages, indexInMemPool);
  size_t index = indexInMemPool;
  indexInMemPool += numPages * PAGE_SIZE;
  assert(indexInMemPool<=sizeMemPool && "time to manage memory chunks in liballoc :D");
  return (char*)startMemPool + index;
}

int liballoc_free(void*,size_t){
  kprintf("liballoc_free unhandled!\n");
  return 0;
}

size_t getTotalKMallocated() { return indexInMemPool; }
} // extern "C"

#ifndef UNIT_TESTS
void *operator new(size_t size) {
  void *r = kmalloc(size);
  return r;
}

void *operator new[](size_t size) {
  void *r = kmalloc(size);
  return r;
}

void operator delete(void *p) { kfree(p); }

void operator delete(void *p, std::size_t ) { kfree(p); }

void operator delete[](void *p) { kfree(p); }

void operator delete[](void *p, std::size_t) { kfree(p); }

#endif