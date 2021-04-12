#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>
#include <bslstl_string.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <string>
#include <memory>

#include "shim_buffermanager.hpp"

#include "fragmenter.hpp"

int main()
{
  //  Fragmenter<16, 16, 1000, 1, 100, true> frag;
  Fragmenter<16, 16, 1000000, 500, 1000, true> frag;
  for (auto i = 0; i < 4; i++) {
    std::cout << malloc(16) << std::endl;
  }
  return 0;
  using namespace BloombergLP;
  bdlma::ShimBufferManager mgr;
  for (auto it = 0; it < 100000; it++) {
    for (auto i = 0; i < 1000; i++) {
      volatile char * ch = (char *) mgr.allocate(16);
      //      std::cout << "ch = " << (void *) ch << std::endl;
    }
    mgr.reset();
  }
  return 0;
}
