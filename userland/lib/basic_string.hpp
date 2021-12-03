#pragma once
#include <stddef.h>

template <class CharT> class basic_string {
public:
  typedef const CharT &const_reference;
  basic_string(CharT *data, size_t size) : _data(data), _size(size) {}
  constexpr const CharT *c_str() const noexcept { return _data; }

  constexpr const_reference operator[](size_t pos) const { return _data[pos]; }
  constexpr size_t size() const noexcept { return _size; }

private:
  CharT *_data;
  size_t _size;
};

using string = basic_string<char>;

template <class CharT>
constexpr bool operator==(const basic_string<CharT> &lhs,
                          const basic_string<CharT> &rhs) noexcept {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.size(); i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}

template <class CharT>
constexpr bool operator==(const basic_string<CharT> &lhs,
                          const char *rhs) noexcept {
  size_t rhsSize = 0;
  for (rhsSize = 0; rhs[rhsSize]; (rhsSize)++)
    ;
  if (lhs.size() != rhsSize) {
    return false;
  }
  for (size_t i = 0; i < lhs.size(); i++) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }
  return true;
}