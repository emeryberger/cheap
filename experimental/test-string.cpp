#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>
#include <bslstl_string.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <bslma_testallocator.h>
#include <bslma_stdallocator.h>
#include <string>
//#include <memory>
#include "shim_allocator.hpp"

#define USE_BUFFERED 0
#define USE_NEWDELETE 0
#define USE_SHIM 1

constexpr auto ITS = 1; // 128;

#if USE_BUFFERED + USE_NEWDELETE + USE_SHIM != 1
#error "Exactly one of USE_BUFFERED, USE_NEWDELETE, or NEW_SHIM must be set to 1."
#endif

#if USE_BUFFERED
using allocatorType = BloombergLP::bdlma::BufferedSequentialAllocator;
#elif USE_NEWDELETE
using allocatorType = BloombergLP::bslma::NewDeleteAllocator;
#elif USE_SHIM
using allocatorType = ShimAllocator;
#endif

template <class T>
class stl_alloc : public bsl::allocator<allocatorType> {
public:
  using value_type = T;
};

#include "fragmenter.hpp"

#include <chrono>

int main()
{
#define SEED 0 // 4148279034// 0 // 99371865
#if 0
  for (auto i = 0; i < 100; i++) {
      volatile Fragmenter<48, 56, 104857, 99, 100, SEED> frag;
  }
#endif
  
  using namespace std::chrono;

  auto start = high_resolution_clock::now();
  
  //  using allocatorType = BloombergLP::bslma::TestAllocator; // BufferedSequentialAllocator;
#if USE_BUFFERED
  char abuf[1024*100];
  allocatorType alloc(abuf, 1024*100);
#elif USE_NEWDELETE
  allocatorType alloc;
#elif USE_SHIM
  allocatorType alloc;
#endif

  
  volatile void * buf[1000];
  volatile char rdBuf[64];

  
  for (int i = 0; i < 10000000 / ITS; i++) {
#if !USE_NEWDELETE
    alloc.rewind();
#endif

#if 0
    
    for (int j = 0; j < 1000; j++) {
      buf[j] = alloc.allocate(64);
      memset((void *) buf[j], 13, 64);
      memcpy((void *) rdBuf, (void *) buf[j], 64);
    }
    for (int j = 0; j < 1000; j++) {
      alloc.deallocate((void *) buf[j]);
    }

#else

    
    for (int j = 0; j < ITS; j++) {
      volatile bsl::string bs1 ("The quick brown fox jumps over the lazy dog", &alloc);
      volatile bsl::string bs2 ("Portez ce vieux whisky au juge blond qui fume", &alloc);
#if 0
      volatile bsl::string bs3 ("Portez ce vieux whisky au juge blond qui fume la pipe", &alloc);
      volatile bsl::string bs4 ("Lorem ipsum dolor sit amet, consectetur adipiscing elit", &alloc);
      volatile bsl::string bs5 ("The quick brown fox jumps over the lazy dog", &alloc);
      volatile bsl::string bs6 ("Portez ce vieux whisky au juge blond qui fume", &alloc);
      volatile bsl::string bs7 ("Portez ce vieux whisky au juge blond qui fume la pipe", &alloc);
      volatile bsl::string bs8 ("Lorem ipsum dolor sit amet, consectetur adipiscing elit", &alloc);
#endif
    }
#endif
#if 0
    vector<int, stl_alloc<int>> v1;
    for (int j = 0; j < 100; j++) {
      v1.push_back(i);
    }
    for (int j = 99; j >= 0; j--) {
      volatile int z = v1.back();
      v1.pop_back();
    }
#endif
  }
  //  std::cout << bs << std::endl;
  //std::cout << alloc.numBlocksInUse() << std::endl;

  auto stop = high_resolution_clock::now();

  std::cout << "Time elapsed: " << (duration_cast<duration<double>>(stop-start)).count() << std::endl;
  return 0;
}
