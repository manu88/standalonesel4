#include "optional.hpp"
#include "tests.hpp"
#include <assert.h>

Optional<int> create(int value) { return Optional<int>(value); }

int testOptional() {
  Optional<bool> a = {};
  assert(a.has_value() == false);

  auto b = Optional<bool>(true);
  assert(b.has_value());
  assert(*b == true);

  auto c = create(42);
  assert(c.has_value());
  assert(*c == 42);

  return 0;
}