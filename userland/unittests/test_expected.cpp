#include "expected.hpp"
#include "tests.hpp"
#include <assert.h>
#include <stdio.h>

int testExpected() {

  auto a = success<float, int>(12.f);
  assert(a.isValid);
  assert(a.value == 12.f);

  auto b = unexpected<bool, int>(42);
  assert(!b);
  assert(b.error == 42);
  return 0;
}