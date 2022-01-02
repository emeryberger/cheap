#if defined(__APPLE__)
#else
#include <malloc.h>
#endif

#include <vector>

void foo() {
  //volatile auto* a = new std::vector<int>;
  volatile auto* b = malloc(16);
  volatile auto* c = calloc(32, 16);
  //  volatile auto* d = realloc((void*) b, 32);
  
  // delete a;
  free((void*) b);
  free((void*) c);
}

void bar() {
  foo();
}

int main() {
  bar();
}
