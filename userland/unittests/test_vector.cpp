#include "tests.hpp"
#include "vector.hpp"
#include <assert.h>
#include <iostream>
#include <stdio.h>

int testVector() {
  printf("Test vector 1\n");
  vector<bool> a;
  assert(a.size() == 0);
  assert(a.empty());
  a.push_back(true);
  a.push_back(false);
  a.push_back(true);
  for (const auto &e : a) {
    std::cout << "Elem " << e << "\n";
  }

  assert(a.size() == 3);
  assert(a[0] == true);
  a.clear();
  assert(a.empty());
  assert(a.size() == 0);
  printf("Test vector 2\n");

  vector<int> b;
  for (int i = 0; i < 100; i++) {
    b.push_back(i);
  }
  assert(b.size() == 100);
  int i = 0;
  for (const auto &j : b) {
    assert(i == j);
    i++;
  }
  b.clear();
  assert(b.empty());
  return 0;
}