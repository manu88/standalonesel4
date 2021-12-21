#pragma once
#ifdef UNIT_TESTS
#include "sel4_mock.hpp"
#else
#include "sel4.hpp"
#endif
#include <stddef.h>

extern "C" {
void setMemoryPool(void *start, size_t size, seL4_CPtr mutexNotif);
size_t getTotalKMallocated(void);
void *kmalloc(size_t size);
void *krealloc(void *ptr, size_t size);
void kfree(void *ptr);
void kfreeWithSize(void *ptr, size_t size);
}