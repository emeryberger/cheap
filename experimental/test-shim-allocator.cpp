#include <string.h>

#include "shim_allocator.hpp"

#define USE_MALLOC 1

#if USE_MALLOC
#warning "Using malloc"
#define MALLOC(x) malloc(x)
#define FREE(x) free(x)
#else
#warning "Using the shim"
#define MALLOC(x) alloc.allocate(x)
#define FREE(x) alloc.deallocate(x)
#endif

int main()
{
  constexpr size_t sz = 8;
  ShimAllocator alloc;
  constexpr int its = 1000000;
  auto ptrs = new void *[its];
  volatile void * ptr = nullptr;
  for (auto its = 0; its < 10000; its++) {
    for (auto i = 0; i < its; i++) {
      ptr = MALLOC(sz);
      ptrs[i] = (void *) ptr;
      memset((void *) ptr, 0, sz);
    }
#if USE_MALLOC
    for (auto i = 0; i < its; i++) {
      FREE(ptrs[i]);
    }
#else
    alloc.release();
#endif
  }
  return (long) ptr;
}
