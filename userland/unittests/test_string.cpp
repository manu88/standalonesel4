#include "basic_string.hpp"
#include "tests.hpp"
#include "vector.hpp"
#include <assert.h>
#include <cstring>
#include <iostream>
#include <stdio.h>

static void testStringBase() {
  string a = "Hello";
  assert(strcmp(a.c_str(), "Hello") == 0);
  assert(a.size() == strlen("Hello"));
  assert(a[0] == 'H');
  assert(a[1] == 'e');
  assert(a[2] == 'l');
  assert(a[3] == 'l');
  assert(a[4] == 'o');
  auto b = a;
  assert(a == b);
}

static void testVectorString() {
  vector<string> v;
  v.push_back("Hello");
  v.push_back("World");
  assert(v.size() == 2);
  assert(v[0] == "Hello");
  assert(v[1] == "World");
}

int testString() {
  printf("Test string\n");
  testStringBase();
  testVectorString();
  return 0;
}