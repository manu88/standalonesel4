#pragma once
#include <stddef.h>

template <typename Type> class vector {
public:
  typedef const Type &const_reference;

  constexpr size_t size() const noexcept { return _size; }

  constexpr void push_back(Type &&value) {}

private:
  size_t _size = 0;
};