#include "kmalloc.hpp"
#include "lib/cstring.h"
#ifdef UNIT_TESTS
#include "sel4_mock.hpp"
#include <cassert>
#else
#include "mutex.h"
#include "runtime.h"
#endif
#include "Platform.hpp"
#include "klog.h"
#include "liballoc.h"
#include <stddef.h>
#include <sys/types.h>
extern "C" {

static void *startMemPool = nullptr;
static size_t sizeMemPool = 0;
static size_t indexInMemPool = 0;
static int hasMemoryPool = 0;
static seL4_CPtr _mutexNotif = 0;
static sync_mutex_t _lock = {};

typedef struct {
  ssize_t index;
  size_t numPages;
} Chunk;
#define NUM_OF_CHINKS 16
static Chunk chunks[NUM_OF_CHINKS];

void setMemoryPool(void *start, size_t size, seL4_CPtr mutexNotif) {
  kprintf("setMemoryPool at %p size=%zi\n", start, size);
  startMemPool = start;
  sizeMemPool = size;
  _mutexNotif = mutexNotif;
  hasMemoryPool = 1;

  if (sync_mutex_init(&_lock, _mutexNotif) != 0) {
    assert(0);
  }

  for (size_t i = 0; i < NUM_OF_CHINKS; i++) {
    chunks[i].index = -1;
    chunks[i].numPages = 0;
  }
}

int liballoc_lock() {
  assert(hasMemoryPool);
  sync_mutex_lock(&_lock);
  return 0;
}

int liballoc_unlock() {
  assert(hasMemoryPool);
  sync_mutex_unlock(&_lock);
  return 0;
}

static void printChunks() {
  kprintf("--> chunks (%zi slots)\n", NUM_OF_CHINKS);
  for (size_t i = 0; i < NUM_OF_CHINKS; i++) {
    if (chunks[i].index != -1) {
      kprintf("%i: 0X%X %i\n", i, chunks[i].index, chunks[i].numPages);
    }
  }
  kprintf("<-- chunks\n");
}

void kmallocPrintStats() { printChunks(); }
static ssize_t tryFindChunk(size_t size) {
  for (size_t i = 0; i < NUM_OF_CHINKS; i++) {
    if (chunks[i].index != -1 && size <= chunks[i].numPages) {
      auto index = chunks[i].index;
      chunks[i].index = -1;
      chunks[i].numPages = 0;
      return index;
    }
  }
  return -1;
}

void *liballoc_alloc(size_t numPages) {
  assert(hasMemoryPool);
  ssize_t potentialIndex = tryFindChunk(numPages);
  if (potentialIndex != -1) {
    return (char *)startMemPool + potentialIndex;
  }
  size_t index = indexInMemPool;
  indexInMemPool += numPages * PAGE_SIZE;
  if (indexInMemPool > sizeMemPool) {
    kprintf("indexInMemPool=%zu sizeMemPool=%zu\n", indexInMemPool,
            sizeMemPool);
  }
  assert(indexInMemPool <= sizeMemPool &&
         "time to manage memory chunks in liballoc :D");
  return (char *)startMemPool + index;
}

static bool tryInsertChunk(size_t pos, size_t size) {
  for (size_t i = 0; i < NUM_OF_CHINKS; i++) {
    if (chunks[i].index == -1) {
      chunks[i].index = (ssize_t)pos;
      chunks[i].numPages = size;
      return true;
    }
  }
  return false;
}

int liballoc_free(void *ptr, size_t size) {
  size_t pos = (size_t)ptr - (size_t)startMemPool;
  size_t sizeInBytes = size * PAGE_SIZE;
  if (indexInMemPool - sizeInBytes == pos) {
    indexInMemPool -= sizeInBytes;
  } else {
    bool found = tryInsertChunk(pos, size);
    if (!found) {
      kprintf("liballoc_free ptr=0X%X size=%zi\n", ptr, size);
      kprintf("pos = 0X%X current = 0X%X start = 0X%X\n", pos, indexInMemPool,
              ((size_t)startMemPool));
      kprintf("liballoc_free should stash freed\n");
      assert(0);
    }
  }
  return 0;
}

extern size_t kmalloced;
size_t getTotalKMallocated() {
  sync_mutex_lock(&_lock);
  auto r = kmalloced;
  sync_mutex_unlock(&_lock);
  return r;
}

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

void operator delete(void *p, std::size_t) { kfree(p); }

void operator delete[](void *p) { kfree(p); }

void operator delete[](void *p, std::size_t) { kfree(p); }

#endif