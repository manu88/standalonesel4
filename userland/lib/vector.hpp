#pragma once
#include <cstdlib>
#include <stddef.h>
#ifdef UNIT_TESTS
#include <cassert>
#else
#include "../kmalloc.hpp"
#include "../runtime.h"
#endif

struct Allocator {
  Allocator() {}
  virtual void *alloc(size_t size) const = 0;
  virtual void free(void *ptr) const = 0;
};

#ifdef UNIT_TESTS
struct Mallocator {
  void *alloc(size_t size) const { return malloc(size); }
  void free(void *ptr) const { ::free(ptr); }
  void *realloc(void *ptr, size_t size) const { return ::realloc(ptr, size); };
};
using DefaultAllocator = Mallocator;
#else
struct KMallocator {
  void *alloc(size_t size) const { return kmalloc(size); }
  void free(void *ptr) const { ::kfree(ptr); }
  void *realloc(void *ptr, size_t size) const { return ::realloc(ptr, size); };
};
using DefaultAllocator = KMallocator;
#endif

template <class Type> struct Iterator {

  Iterator(Type *i) : _item(i) {}
  Type *_item;
  Iterator operator++() {
    _item += 1;
    return *this;
  }
  bool operator!=(const Iterator &rhs) { return _item != rhs._item; }

  Type &operator*() const { return *_item; }
};

template <typename Type, class AllocatorType = DefaultAllocator> class vector {
public:
  typedef const Type &const_reference;
  typedef Iterator<Type> iterator;
  typedef const Iterator<Type> const_iterator;
  vector(const AllocatorType &allocator = DefaultAllocator())
      : _allocator(allocator) {}
  vector(size_t count, const AllocatorType &allocator = DefaultAllocator())
      : vector(allocator) {
    _reservation = count;
  }

  ~vector() {
    if (_data) {
      _allocator.free(_data);
    }
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

  constexpr size_t size() const noexcept { return _size; }

  constexpr void push_back(const Type &value) {
    doRealloc();
    _data[_size++] = value;
  }

  constexpr void push_back(Type &&value) {
    doRealloc();
    _data[_size++] = value;
  }
  constexpr void clear() noexcept { _size = 0; }
  constexpr const_reference operator[](size_t pos) const { return _data[pos]; }

  constexpr const_iterator begin() noexcept { return const_iterator(_data); };
  constexpr const_iterator end() noexcept {
    return const_iterator(_data + _size);
  };

private:
  void doRealloc() {
    if (!_data) {
      if (_reservation == 0) {
        _reservation = 10;
      }
      auto sizeToAlloc = _reservation * sizeof(Type);
      _data = reinterpret_cast<Type *>(_allocator.alloc(sizeToAlloc));
      assert(_data);
    }
    if (_size >= _reservation) {
      _reservation += 10;
      auto sizeToRealloc = _reservation * sizeof(Type);
      _data =
          reinterpret_cast<Type *>(_allocator.realloc(_data, sizeToRealloc));
    }
  }
  const AllocatorType &_allocator;
  size_t _size = 0;
  size_t _reservation = 0;
  Type *_data = nullptr;
};