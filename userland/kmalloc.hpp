#pragma once
#include "sel4.hpp"
#include <stddef.h>

extern "C" {
void setMemoryPool(void *start, size_t size, seL4_CPtr mutexNotif);
size_t getTotalKMallocated(void);
void *kmalloc(size_t size);
void *krealloc(void *ptr, size_t size);
void kfree(void *ptr);
}