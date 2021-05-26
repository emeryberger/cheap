#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <random>
#include <string.h>
//#include <bslstl_string.h>
//#include <bdlma_bufferedsequentialallocator.h>
#include <bdlma_buffermanager.h>
#include <string>
#include <cstring>
#include <memory>
#include <chrono>
#include <ratio>
#include <ctime>

#include "shim_buffermanager.hpp"

#include "shuffler.hpp"

#if !defined(USE_BUFFER) && !defined(USE_SHIM)
#define USE_BUFFER 1
#define USE_SHIM 0
#endif

#if USE_BUFFER + USE_SHIM != 1
#error "One must be chosen"
#endif

#if !defined(SHUFFLE)
//#define SHUFFLE false
#define SHUFFLE true
#endif

int main()
{
  constexpr int WorkingSet = 256000; // was 64000
  constexpr int ObjectSize = 64; // was 64
  constexpr int Iterations = WorkingSet / ObjectSize;
  // Shuffler<ObjectSize, ObjectSize, 1000000, 999, 1000, SHUFFLE> frag;
  //  Shuffler<ObjectSize, ObjectSize, 1000000, 100, 1000, SHUFFLE> frag;
  //  Shuffler<ObjectSize, ObjectSize, 1000000, 50, 1000, SHUFFLE> frag;
  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  volatile Shuffler<ObjectSize, ObjectSize, 1000000, 100, 1000, SHUFFLE> frag;
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  
  // Shuffler<ObjectSize, ObjectSize, 1000000, 100, 1000, SHUFFLE> frag;
  //Shuffler<ObjectSize, ObjectSize, 1000000, 50, 1000, SHUFFLE> frag;
  //  std::cout << "starting " << getpid() << std::endl;

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
  for (auto it = 0; it < 100000; it++) {
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
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t1);
  std::cout << "Time elapsed = " << time_span2.count() - time_span.count() << std::endl;
  return 0;
}
