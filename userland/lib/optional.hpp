#pragma once

template <typename T> class Optional {
public:
  constexpr Optional() noexcept : _has_value(false) {}
  explicit Optional(T value) : value_(value), _has_value(true) {}
  ~Optional() {}
  constexpr bool has_value() const noexcept { return _has_value; }
  constexpr const T &operator*() const &noexcept { return value_; }
  constexpr T &operator*() &noexcept { return value_; }

  T* operator->(){
    return &value_;
  }

  const T* operator->() const {
    return &value_;
  }

  operator bool() const noexcept { return _has_value; }

private:
  struct empty_byte {};
  union {
    empty_byte empty_;
    T value_;
  };
  bool _has_value;
};