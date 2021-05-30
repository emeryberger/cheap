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

#include "cxxopts.hpp"

int main(int argc, char * argv[])
{
  cxxopts::Options options(argv[0], " - command line options");
  options.add_options()
    ("shuffle","Shuffle")
    ("buffer","Buffer")
    ("shim","Shim")
    ("working-set","Working set", cxxopts::value<int>())
    ("locality-iterations","Locality iterations", cxxopts::value<int>());

  auto result = options.parse(argc, argv);
  
  int WorkingSet = 256000; // was 64000

  if (result.count("working-set")) {
    WorkingSet = result["working-set"].as<int>();
  }
  
  constexpr int ObjectSize = 64; // was 64
  int Iterations = WorkingSet / ObjectSize;
  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  volatile Shuffler<ObjectSize/2, ObjectSize, 1000000, 100, 1000> frag (result.count("shuffle"));
  
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  
  if (result.count("buffer")) {
    std::cout << "using BufferManager ";
  } else {
    std::cout << "using ShimBufferManager ";
  }

  int localityIterations = 1;
  if (result.count("locality-iterations")) {
    localityIterations = result["locality-iterations"].as<int>();
  }
  
  if (result.count("shuffle")) {
    std::cout << "(shuffled) ";
  }
  
  std::cout << std::endl;

  using namespace BloombergLP;
  char * buf = new char[Iterations * ObjectSize];
  void ** ptrs = new void * [Iterations];
  //    for (auto it = 0; it < 10000000; it++) {
  BloombergLP::bdlma::BufferManager mgr_buffer(buf, Iterations*ObjectSize);
  bdlma::ShimBufferManager mgr_shim(nullptr, Iterations);
  
  for (auto it = 0; it < 1000; it++) {
    
    for (auto i = 0; i < Iterations; i++) {
      volatile char * ch;
      if (result.count("buffer")) {
	ch = (char *) mgr_buffer.allocate(ObjectSize);
      } else {
	ch = (char *) mgr_shim.allocate(ObjectSize);
      }
      memset((void *) ch, 13, ObjectSize);
      ptrs[i] = (void *) ch;
    }
    volatile char ch;
    for (auto it = 0; it < localityIterations; it++) {
      for (auto i = 0; i < Iterations; i++) {
	volatile char bufx[ObjectSize];
	memcpy((void *) bufx, (void *) ptrs[i], ObjectSize);
	ch = buf[ObjectSize-1];
      }
    }

    mgr_buffer.release();
    mgr_shim.release();
  }
  delete [] ptrs;
  delete [] buf;
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t1);
  std::cout << "Time elapsed = " << time_span2.count() - time_span.count() << std::endl;
  return 0;
}
