#include <algorithm>
#include <bdlma_buffermanager.h>
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <random>
#include <ratio>
#include <string.h>
#include <string>
#include <unistd.h>

#include "litterer.hpp"
#include "page-litterer.hpp"
#include "shim_buffermanager.hpp"

#include "cxxopts.hpp"

int main(int argc, char* argv[]) {
  cxxopts::Options options(argv[0], "");
  options.add_options()
    ("help","Print command-line options")
    ("shuffle","Shuffle ('litter') randomly before starting")
    ("page-litter-v1","(V1) Adversary case. If set, overrides all other litter options.")
    ("page-litter-v2","(V2) Adversary case. If set, overrides all other litter options.")
    ("seed","Use this seed for shuffling", cxxopts::value<size_t>())
    ("buffer","Use the actual buffer implementation")
    ("shim","Use a shim buffer implementation")
    ("object-size","Size of objects to allocate", cxxopts::value<int>())
    ("bytes-to-read", "Bytes to read of each object (in locality iterations).", cxxopts::value<int>())
    ("working-set","Size of working set (in bytes)", cxxopts::value<int>())
    ("loops","Number of loops", cxxopts::value<int>())
    ("litter-objects","Number of objects to litter", cxxopts::value<int>())
    ("litter-occupancy", "Occupancy after littering (between 0 and 1).", cxxopts::value<float>())
    ("locality-iterations","Locality iterations (non-allocating)", cxxopts::value<int>());

  auto result = options.parse(argc, argv);

  if (result.count("help")) {
    std::cout << options.help({""}) << std::endl;
    return 0;
  }

  int ObjectSize = 256; // was 64
  if (result.count("object-size")) {
    ObjectSize = result["object-size"].as<int>();
  }

  int WorkingSet = 10000000; // was 64000
  if (result.count("working-set")) {
    WorkingSet = result["working-set"].as<int>();
  }
  int Iterations = WorkingSet / ObjectSize;

  int Loops = 10;
  if (result.count("loops")) {
    Loops = result["loops"].as<int>();
  }

  float litterOccupancy = 0.3;
  if (result.count("litter-occupancy")) {
    litterOccupancy = result["litter-occupancy"].as<float>();
  }

  int litterObjects = WorkingSet;
  if (result.count("litter-objects")) {
    litterObjects = result["litter-objects"].as<int>();
  }

  using namespace std::chrono;
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  size_t seed = 0;
  if (result.count("seed")) {
    seed = result["seed"].as<size_t>();
  } else {
    std::random_device rd;
    seed = rd();
  }

  int BytesToRead = ObjectSize;
  if (result.count("bytes-to-read")) {
    BytesToRead = result["bytes-to-read"].as<int>();
  }

  if (result.count("buffer")) {
    std::cout << "using BufferManager ";
  } else {
    std::cout << "using ShimBufferManager ";
  }

  int localityIterations = 1000;
  if (result.count("locality-iterations")) {
    localityIterations = result["locality-iterations"].as<int>();
  }

  if (result.count("page-litter-v2")) {
    std::cout << "(page-litter-v2: seed = " << seed << ")" << std::endl;
    volatile PageLittererV2 frag(ObjectSize, ObjectSize, Iterations, seed);
  } else if (result.count("page-litter-v1")) {
    std::cout << "(page-litter-v1: seed = " << seed << ")" << std::endl;
    volatile PageLittererV1 frag(ObjectSize, ObjectSize, Iterations, seed);
  } else {
    if (result.count("shuffle")) {
      std::cout << "(shuffled: seed = " << seed << ") " << std::endl;
    }
    volatile Litterer frag(ObjectSize, ObjectSize, litterObjects, (int) (litterOccupancy * 100), 100,
                           result.count("shuffle"), seed);
  }

  std::cout << "working set = " << WorkingSet << " bytes " << std::endl;
  std::cout << "object size = " << ObjectSize << std::endl;
  std::cout << "bytes to read = " << BytesToRead << std::endl;
  std::cout << "litter occupancy = " << litterOccupancy << std::endl;
  std::cout << "loops = " << Loops << std::endl;
  std::cout << "locality iterations = " << localityIterations << std::endl;
  std::cout << "iterations = " << Iterations << std::endl;

  using namespace BloombergLP;
  char* buf = new char[Iterations * ObjectSize];
  auto ptrs = new volatile void*[Iterations];
  BloombergLP::bdlma::BufferManager mgr_buffer(buf, Iterations * ObjectSize);
  bdlma::ShimBufferManager mgr_shim(nullptr, Iterations);
  auto which_buf = result.count("buffer");
  if (which_buf) {
    std::cout << "buffer starts at " << (void*) buf << std::endl;
  }

  // Start the actual benchmark.

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  for (volatile auto it = 0; it < Loops; it++) {

    for (volatile auto i = 0; i < Iterations; i++) {
      volatile char* ch;
      volatile int value = 13;
      if (which_buf) {
        ch = (char*) mgr_buffer.allocate(ObjectSize);
      } else {
        ch = (char*) mgr_shim.allocate(ObjectSize);
        if (i <= 1) {
          // std::cout << "object " << i << " = " << (void*) ch << std::endl;
        }
      }
      memset((void*) ch, value, ObjectSize);
      ptrs[i] = (void*) ch;
    }
    volatile char ch;
    for (volatile auto it = 0; it < localityIterations; it++) {
      for (volatile auto i = 0; i < Iterations; i++) {
        volatile char bufx[ObjectSize];
        memcpy((void*) bufx, (void*) ptrs[i], BytesToRead);
        ch += bufx[ObjectSize - 1];
      }
    }

    mgr_buffer.release();
    mgr_shim.release();
  }
  delete[] ptrs;
  delete[] buf;
  high_resolution_clock::time_point t3 = high_resolution_clock::now();
  duration<double> time_span2 = duration_cast<duration<double>>(t3 - t1);
  std::cout << "Time elapsed = " << time_span2.count() - time_span.count() << std::endl;
  return 0;
}
