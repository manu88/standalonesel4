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

extern "C" {

void kfree_no_lock(void *ptr, size_t size);
void *kmalloc_no_lock(size_t size);

union chunk;
struct info {
  union chunk *prev;
  union chunk *next;
  size_t count;
  size_t size;
};

struct node {
  struct info info;
};

union chunk {
  unsigned char data[sizeof(struct node)];
  struct node node;
};

// static memory
enum { CHUNK_MAX = 1024 };
static size_t initialized = 0;
static size_t chunk_max = 0;
static union chunk *pool = NULL; //[CHUNK_MAX];
static union chunk head = {.node = {{NULL, NULL, 0, 0}}};
static union chunk tail = {.node = {{NULL, NULL, 0, 0}}};
static size_t _totalAllocated = 0;

int hasMemoryPool = 0;
static seL4_CPtr _mutexNotif = 0;
static sync_mutex_t _lock = {};

void setMemoryPool(void *start, size_t size, seL4_CPtr mutexNotif) {
  pool = reinterpret_cast<union chunk *>(start);
  chunk_max = size / sizeof(union chunk);
  _mutexNotif = mutexNotif;
  hasMemoryPool = 1;

  if (sync_mutex_init(&_lock, _mutexNotif) != 0) {
    assert(0);
  }
}

size_t getTotalKMallocated() { return _totalAllocated; }

static void init() {
  head.node.info.next = pool;
  union chunk *prev = &head;
  union chunk *curr = pool;
  for (size_t i = 0; i < chunk_max; i++) {
    curr->node.info.prev = prev;
    curr->node.info.next = curr + 1;
    curr->node.info.count = 0;
    //    curr->node.used = 0;
    prev = curr++;
  }
  prev->node.info.next = &tail;
  tail.node.info.prev = prev;
  initialized = 1;
}

static size_t chunk_count(size_t size) {
  size_t chunks = 0;
  size_t chunks_size = 0;
  while (chunks_size < size + sizeof(struct info)) {
    chunks_size += sizeof(union chunk);
    chunks++;
  }
  return chunks;
}

static void *unlink(union chunk *top, size_t count, size_t size) {
  union chunk *prev = top->node.info.prev;
  union chunk *next = top[count - 1].node.info.next;
  prev->node.info.next = next;
  next->node.info.prev = prev;
  top->node.info.count = count;
  top->node.info.size = size;
  _totalAllocated += size;
  return top->data + sizeof(struct info);
}

static void relink(union chunk *top, size_t expectedSize) {
  if (top < pool || pool + chunk_max - 1 < top) {
    return;
  }
  if (expectedSize > 0) {
    if (expectedSize == top->node.info.size) {
      kprintf(
          "expecting to free %zi bytes, but chunk size is %zi. will assert\n",
          expectedSize, top->node.info.size);
    }
    assert(expectedSize == top->node.info.size);
  }
  _totalAllocated -= top->node.info.size;
  union chunk *prev = &head;
  while (prev->node.info.next != &tail && prev->node.info.next < top) {
    prev = prev->node.info.next;
  }
  if (prev == top)
    return; // dual free
  union chunk *next = prev->node.info.next;
  union chunk *curr = top;
  prev->node.info.next = curr;
  size_t count = top->node.info.count;
  for (size_t i = 0; i < count; i++) {
    curr->node.info.prev = prev;
    curr->node.info.next = curr + 1;
    curr->node.info.count = 0;
    prev = curr++;
  }
  prev->node.info.next = next;
  next->node.info.prev = prev;
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr) {
    void *r = kmalloc(size);
    return r;
  }
  if(ptr && size ==0){
    kfree(ptr);
    return NULL;
  }
  sync_mutex_lock(&_lock);
  char *bptr = (char *)ptr;
  ptr = bptr - sizeof(struct info);
  union chunk *chunk = (union chunk *)ptr;
  size_t prevSize = chunk->node.info.size;
  if (size < prevSize) {
    sync_mutex_unlock(&_lock);
    return nullptr;
  }
  void *newPtr = kmalloc_no_lock(size);
  if (!newPtr) {
    sync_mutex_unlock(&_lock);
    return nullptr;
  }
  memcpy(newPtr, ptr, prevSize);
  memset(((char *)newPtr) + prevSize, 0, size - prevSize);
  kfree_no_lock(ptr, 0);
  sync_mutex_unlock(&_lock);
  return newPtr;
}

void *kmalloc(size_t size) {
  sync_mutex_lock(&_lock);
  void *r = kmalloc_no_lock(size);
  sync_mutex_unlock(&_lock);
  return r;
}

void *kmalloc_no_lock(size_t size) {
  if (!initialized){
    init();
  }
  size_t chunks = chunk_count(size);
  size_t keep = 0;
  union chunk *top = head.node.info.next;
  for (union chunk *curr = top; curr != &tail; curr = curr->node.info.next) {
    keep++;
    if (keep == chunks)
      return unlink(top, chunks, size);
    if (curr->node.info.next == curr + 1)
      continue;
    keep = 0;
    top = curr->node.info.next;
  }
  return NULL;
}

void kfreeWithSize(void *ptr, size_t size) {
  sync_mutex_lock(&_lock);
  kfree_no_lock(ptr, size);
  sync_mutex_unlock(&_lock);
}

void kfree(void *ptr) {
  sync_mutex_lock(&_lock);
  kfree_no_lock(ptr, 0);
  sync_mutex_unlock(&_lock);
}

void kfree_no_lock(void *ptr, size_t size) {
  if (ptr == NULL)
    return;
  char *bptr = (char *)ptr;
  ptr = bptr - sizeof(struct info);
  relink((union chunk *)ptr, size);
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

void operator delete(void *p, std::size_t size) { kfreeWithSize(p, size); }

void operator delete[](void *p) { kfree(p); }

void operator delete[](void *p, std::size_t size) { kfreeWithSize(p, size); }

#endif