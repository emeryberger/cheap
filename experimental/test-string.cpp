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

int main()
{
  //  using allocatorType = BloombergLP::bslma::TestAllocator; // BufferedSequentialAllocator;
  for (int i = 0; i < 100000; i++) {
#if USE_BUFFERED
    char buf[1024*100];
    allocatorType alloc(buf, 1024*100);
#elif USE_NEWDELETE
    allocatorType alloc;
#elif USE_SHIM
    allocatorType alloc;
#endif

    for (int j = 0; j < 100; j++) {
      volatile bsl::string bs1 ("The quick brown fox jumps over the lazy dog", &alloc);
      volatile bsl::string bs2 ("Portez ce vieux whisky au juge blond qui fume", &alloc);
      volatile bsl::string bs3 ("Portez ce vieux whisky au juge blond qui fume la pipe", &alloc);
      volatile bsl::string bs4 ("Lorem ipsum dolor sit amet, consectetur adipiscing elit", &alloc);
    }
    vector<int, stl_alloc<int>> v1;
    for (int j = 0; j < 100; j++) {
      v1.push_back(i);
    }
    for (int j = 99; j >= 0; j--) {
      volatile int z = v1.back();
      v1.pop_back();
    }
  }
  //  std::cout << bs << std::endl;
  //std::cout << alloc.numBlocksInUse() << std::endl;
  return 0;
}
