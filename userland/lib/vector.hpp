#pragma once
#include <cstring> // memmove
#include <stddef.h>
#ifdef UNIT_TESTS
#include <cassert>
#include <cstdlib>
#define kmalloc malloc
#define krealloc realloc
#define kfree free
#else
#include "../kmalloc.hpp"
#endif

template <class Type> struct Iterator {

  Iterator(Type *i, size_t index) : _item(i), _index(index) {}

  constexpr Iterator operator++() {
    _item += 1;
    _index += 1;
    return *this;
  }

  constexpr Iterator operator++(int) {
    _item += 1;
    _index -= 1;
    return Iterator(_item - 1, _index);
  }

  bool operator!=(const Iterator &rhs) {
    return _item != rhs._item && _index != rhs._index;
  }

  constexpr Iterator operator+(ptrdiff_t delta) const {
    return Iterator{_item + delta, _index + delta};
  }
  constexpr Iterator operator-(ptrdiff_t delta) const {
    return Iterator{_item - delta, _index - delta};
  }

  constexpr ptrdiff_t operator-(Iterator rhs) const {
    return static_cast<ptrdiff_t>(_index) - rhs._index;
  }

  Type &operator*() const { return *_item; }

  Type *_item;
  size_t _index;
};

template <typename Type> class vector {
public:
  typedef const Type &const_reference;
  typedef Iterator<Type> iterator;
  typedef const Iterator<Type> const_iterator;
  vector() {}
  vector(size_t count) : vector() { _reservation = count; }

  ~vector() {
    if (_data) {
      kfree(_data);
    }
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

  constexpr iterator erase(size_t pos) {
    return erase(const_iterator(_data + pos, pos));
  }
  constexpr iterator erase(const_iterator pos) {
    iterator return_iterator = pos;
    if (size() == 0) {
      return return_iterator;
    }
    const size_t dist = pos - begin();
    if (dist == size()) {
      return pos;
    }
    if (dist == size() - 1) {
      _size--;
      return pos;
    }
    _size--;
    if (size() == 0) {
      return return_iterator;
    }

    memmove(_data + dist, pos._item + 1, size() * sizeof(Type));
    return return_iterator;
  }

  constexpr size_t size() const noexcept { return _size; }

  constexpr void push_back(const Type &value) {
    doRealloc();
    _data[_size++] = value;
  }

  constexpr void push_back(Type &&value) {
    doRealloc();
    _data[_size++] = value;
  }

  constexpr void pop_back() { erase(end() - 1); }
  constexpr void clear() noexcept { _size = 0; }
  constexpr const_reference operator[](size_t pos) const { return _data[pos]; }

  constexpr Type &operator[](size_t pos) { return _data[pos]; }

  constexpr const_reference front() const noexcept { return _data[0]; }
  constexpr const_reference back() const noexcept { return _data[_size - 1]; }

  constexpr const_iterator begin() noexcept {
    return const_iterator(_data, 0);
  };
  constexpr const_iterator end() noexcept {
    return const_iterator(_data + _size, _size);
  };

  constexpr const_iterator begin() const noexcept {
    return const_iterator(_data, 0);
  };
  constexpr const_iterator end() const noexcept {
    return const_iterator(_data + _size, _size);
  };

private:
  void doRealloc() {
    if (_data == nullptr || _size >= _reservation) {
      if (_reservation == 0) {
        _reservation = 10;
      } else {
        _reservation = _size * 2;
      }
      auto sizeToRealloc = _reservation * sizeof(Type);
      auto newData = reinterpret_cast<Type *>(kmalloc(sizeToRealloc));
      memcpy(newData, _data, _size*sizeof(Type));
      kfree(_data);
      _data = newData;
    }
  }
  size_t _size = 0;
  size_t _reservation = 0;
  Type *_data = nullptr;
};