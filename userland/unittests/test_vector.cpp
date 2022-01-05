#include "tests.hpp"
#include "vector.hpp"
#include <assert.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <vector>

void testStd() {
  printf("testStd\n");

  std::vector<int> a;
  a.push_back(1);
  a.erase(a.end() - 1);
  assert(a.empty());
}
struct Element {
  int i = 0;

  Element(int i = 0) : i(i) {}
};

static void testVectorStruct() {
  vector<Element> a;
  a.push_back(Element());
  for (const auto &e : a) {
  }
}

static void testVectorRemove() {
  printf("Test remove begin() from vector empty\n");
  vector<std::shared_ptr<Element>> vec;
  vec.erase(vec.begin());
  assert(vec.empty());
  auto a = std::shared_ptr<Element>(new Element());
  vec.push_back(a);

  assert(vec.size() == 1);
  assert(vec[0] == a);
  printf("Test remove begin() from vector 1 element\n");
  vec.erase(vec.begin());
  assert(vec.empty());
  bool neverSet = false;
  for (const auto &e : vec) {
    neverSet = true;
  }
  assert(neverSet == false);
  printf("Test remove begin() from vector many element\n");
  for (int i = 0; i < 10; i++) {
    auto a = std::shared_ptr<Element>(new Element(i));
    vec.push_back(a);
    printf("Added %i at %i\n", i, i);
  }
  assert(vec.size() == 10);
  vec.erase(vec.begin());
  printf("First val = %i\n", vec[0]->i);
  assert(vec[0]->i == 1);
  assert(vec.size() == 9);
  printf("----After removing 1st element\n");
  for (const auto &e : vec) {
    printf("%i\n", e->i);
  }
  printf("----\n");

  printf("Test remove end() from vector many element\n");
  vec.erase(vec.end() - 1);
  printf("----\n");
  for (const auto &e : vec) {
    printf("%i\n", e->i);
  }
  printf("----\n");
  assert(vec.size() == 8);
  printf("Last val = %i\n", vec[7]->i);
  assert(vec[7]->i == 8);
}

static void testVectorPopBack() {
  vector<size_t> v;
  v.push_back(100);
  assert(v.back() == 100);
  v.pop_back();
  assert(v.empty());
}

static void testVectorRemove2() {
  printf("testVectorRemove2\n");
  vector<int> v;
  v.push_back(1);
  v.push_back(1);
  v.push_back(0);
  v.push_back(2);
  v.push_back(2);
  v.erase(2);
  assert(v.size() == 4);
  for (const auto &i : v) {
    printf("%i\n", i);
  }
}

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
  assert(i == 100);
  b.clear();
  assert(b.empty());
  testVectorStruct();
  testVectorRemove();
  testVectorRemove2();
  testVectorPopBack();
  // testStd();
  return 0;
}