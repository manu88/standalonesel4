#pragma once
#include <stddef.h>

extern "C" {
void setMemoryPool(void *start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
}