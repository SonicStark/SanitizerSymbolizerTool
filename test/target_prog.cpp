/**
 * Migrated from 
 * compiler-rt/test/sanitizer_common/TestCases/symbolize_pc_inline.cpp
 * and
 * compiler-rt/test/sanitizer_common/TestCases/symbolize_stack.cpp
*/

#include <cstdio>
#include <cstring>
#include <vector>

char buffer[10000];

__attribute__((noinline)) static void C1() {
  for (char *p = buffer; std::strlen(p); p += std::strlen(p) + 1)
    std::printf("%s\n", p);
}

static inline void C2() { C1(); }

static inline void C3() { C2(); }

static inline void C4() { C3(); }

template <int N> struct A {
  template <class T> void RecursiveTemplateFunction(const T &t);
};

template <int N>
template <class T>
__attribute__((noinline)) void A<N>::RecursiveTemplateFunction(const T &) {
  std::vector<T> t;
  return A<N - 1>().RecursiveTemplateFunction(t);
}

template <>
template <class T>
__attribute__((noinline)) void A<0>::RecursiveTemplateFunction(const T &) {
  std::printf("wtf!");
}

int main()
{
  C4();
  std::printf(":-)");
  A<10>().RecursiveTemplateFunction(0);
  return 0;
}