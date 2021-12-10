#pragma once
#include <stddef.h>
#ifdef UNIT_TESTS
#include <cassert>
#include <cstdlib>
#define kmalloc malloc
#define krealloc realloc
#define kfree free
#else
#include "../kmalloc.hpp"
#include "../runtime.h"
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

#if 0
  constexpr iterator erase( const_iterator it ){
    iterator copy_iterator = it; // used to shift elements one position to the left past the deleted element.
    iterator return_iterator = it; // holds original it position, per std implementation

    while (it != (this -> end() - 1)) { // copies elements one position left
        *copy_iterator++ = *++it;
    }

    //alloc.destroy(it); // destroy last element in vector
    //avail = it; // shortens the vector by 1

    return return_iterator;
  }
#endif
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

  constexpr const_reference front() const noexcept{ return _data[0]; }
  constexpr const_reference back() const noexcept{ return _data[_size - 1]; }

  constexpr const_iterator begin() noexcept { return const_iterator(_data); };
  constexpr const_iterator end() noexcept {
    return const_iterator(_data + _size);
  };

  constexpr const_iterator begin() const noexcept { return const_iterator(_data); };
  constexpr const_iterator end() const noexcept {
    return const_iterator(_data + _size);
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
      _data = reinterpret_cast<Type *>(krealloc(_data, sizeToRealloc));
    }
  }
  size_t _size = 0;
  size_t _reservation = 0;
  Type *_data = nullptr;
};