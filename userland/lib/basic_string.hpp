#pragma once
#ifndef UNIT_TESTS
#include "cstring.h"
#else
#include <cstring>
#endif
#include <stddef.h>

template <class CharT> class basic_string {
public:
  static const size_t npos = -1;

  typedef const CharT &const_reference;
  basic_string(const CharT *data)
      : basic_string(const_cast<CharT *>(data), strlen(data)) {}
  basic_string(CharT *data, size_t size) : _data(data), _size(size) {}
  constexpr const CharT *c_str() const noexcept { return _data; }

  constexpr const_reference operator[](size_t pos) const { return _data[pos]; }
  constexpr size_t size() const noexcept { return _size; }
  constexpr bool starts_with(const CharT *s) const {
    if (s == nullptr) {
      return false;
    }
    size_t sSize = 0;
    for (sSize = 0; s[sSize]; (sSize)++) {
      if (sSize > size()) {
        return false;
      }
      if (s[sSize] != _data[sSize]) {
        return false;
      }
    }
    return true;
  }

  constexpr basic_string substr(size_t pos = 0, size_t count = npos) const {
    if (count == npos) {
      return basic_string(_data + pos, _size - pos);
    }
    return basic_string(_data + pos, _size - count - pos);
  }

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
  const size_t rhsSize = strlen(rhs);
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

template <class CharT>
constexpr bool operator==(const char *lhs,
                          const basic_string<CharT> &rhs) noexcept {
  return rhs == lhs;
}