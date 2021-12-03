#include "tests.hpp"
#include "vector.hpp"
#include <assert.h>
#include <stdio.h>

int testVector() {
  printf("Test vector\n");
  vector<bool> a;
  assert(a.size() == 0);
  a.push_back(true);
  assert(a.size() == 1);

  return 0;
}