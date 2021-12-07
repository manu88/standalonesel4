#include "tests.hpp"
#include <stdio.h>

extern "C" int main() {
  printf("Test Expected\n");
  testVector();
  testString();
  testExpected();
  testOptional();
  return 0;
}