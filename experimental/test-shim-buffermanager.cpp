#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>
#include <bslstl_string.h>
#include <bdlma_bufferedsequentialallocator.h>
#include <string>
#include <cstring>
#include <memory>

#include "shim_buffermanager.hpp"

#include "shuffler.hpp"

#if !defined(USE_BUFFER) && !defined(USE_SHIM)
#define USE_BUFFER 0
#define USE_SHIM 1
#endif

#if USE_BUFFER + USE_SHIM != 1
#error "One must be chosen"
#endif

#if !defined(SHUFFLE)
#define SHUFFLE false
//#define SHUFFLE true
#endif

// no shim, no shuffle: 0.07MB (mimalloc), 2.543s
// no shim, shuffle: 0.07MB (mimalloc), 2.554s
// shim, no shuffle: 6.05MB (mimalloc), 5.379s
// shim, shuffle (5%, 1M obj): 62.05MB (mimalloc), 24.852s

int main()
{
  constexpr int ObjectSize = 64;
  constexpr int Iterations = 1000;
  // Shuffler<ObjectSize, ObjectSize, 1000000, 999, 1000, SHUFFLE> frag;
  // Shuffler<ObjectSize, ObjectSize, 1000000, 500, 1000, SHUFFLE> frag;
  Shuffler<ObjectSize, ObjectSize, 1000000, 100, 1000, SHUFFLE> frag;
  //Shuffler<ObjectSize, ObjectSize, 1000000, 50, 1000, SHUFFLE> frag;
  std::cout << "starting " << getpid() << std::endl;

#if USE_BUFFER
  std::cout << "  using BufferManager ";
#elif USE_SHIM
  std::cout << "  using ShimBufferManager ";
#endif

#if SHUFFLE
  std::cout << "(shuffled) ";
#elif USE_SHIM
#endif
  
  std::cout << std::endl;
  
  using namespace BloombergLP;
  char * buf = new char[Iterations * ObjectSize];
  //    for (auto it = 0; it < 10000000; it++) {
  for (auto it = 0; it < 1000000; it++) {
#if USE_BUFFER
    BloombergLP::bdlma::BufferManager mgr(buf, Iterations*ObjectSize);
#elif USE_SHIM
    bdlma::ShimBufferManager mgr(nullptr, Iterations);
#endif
    for (auto i = 0; i < Iterations; i++) {
      volatile char * ch = (char *) mgr.allocate(ObjectSize);
      memset((void *) ch, 13, ObjectSize);
      volatile char buf[ObjectSize];
      memcpy((void *) buf, (void *) ch, ObjectSize);
    }
  }
  delete [] buf;
  return 0;
}
