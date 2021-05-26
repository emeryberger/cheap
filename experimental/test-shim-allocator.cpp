#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>

#include "shim_allocator.hpp"

#ifndef USE_MALLOC
#define USE_MALLOC 0
#endif

#ifndef USE_SHUFFLE
#define USE_SHUFFLE 1
#endif

#if USE_MALLOC
#define MALLOC(x) malloc(x)
#define FREE(x) free(x)
#else
#define MALLOC(x) alloc.allocate(x)
#define FREE(x) alloc.deallocate(x)
#endif


int main()
{
  constexpr size_t objectSize = 64;
  constexpr int its = 1000000;
  constexpr int reps = 10000;

#if USE_MALLOC
  std::cout << "Malloc "; 
#else
  std::cout << "Shim   ";
#endif
  std::cout << " objectSize=" << objectSize << ", reps = " << reps << ", each rep: iterations=" << its;

#if USE_SHUFFLE
  std::cout << " [shuffled]";
#endif
  std::cout << std::endl;
  
  BloombergLP::bslma::ShimAllocator alloc;
  auto ptrs = new void *[its];


  std::random_device rdev;
  std::mt19937 rng(rdev());
  
  // Shuffle memory.
  for (auto i = 0; i < its; i++) {
    ptrs[i] = MALLOC(objectSize);
  }
#if USE_SHUFFLE
  std::shuffle(ptrs, ptrs + its, rng);
#endif
  for (auto i = 0; i < its; i++) {
    FREE(ptrs[i]);
  }
  
  // Now repeatedly allocate and free.
  volatile void * ptr = nullptr;
  volatile char target[objectSize];
  for (auto its = 0; its < reps; its++) {
    for (auto i = 0; i < its; i++) {
      ptr = MALLOC(objectSize);
      ptrs[i] = (void *) ptr;
      memset((void *) ptr, 0, objectSize);
    }
    for (auto i = 0; i < its; i++) {
      memcpy((void *) &target, (void *) ptrs[i], objectSize);
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
