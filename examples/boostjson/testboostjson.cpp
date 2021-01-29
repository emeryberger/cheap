#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

#define OPTIMIZED_HEAP 0 // NB: NOT working

#if CHEAPEN
#include "cheap.h"
#endif

// #include <boost/json/src.hpp>
#include "src.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include <memory_resource>

// From https://github.com/lefticus/cpp_weekly/blob/master/PMR/json_tests.cpp

struct RapidJSONPMRAlloc
{
  std::pmr::memory_resource *upstream = std::pmr::get_default_resource();

  static constexpr bool kNeedFree = true;

  static constexpr auto objectOffset = alignof(std::max_align_t);
  static constexpr auto memPadding = objectOffset * 2;

  void *Malloc(size_t size)
  {
    if (size != 0) {
      const auto allocated_size = size + memPadding;
      std::byte *newPtr = static_cast<std::byte *>(upstream->allocate(allocated_size));
      auto *ptrToReturn = newPtr + memPadding;
      // placement new a pointer to ourselves at the first memory location
      new (newPtr)(RapidJSONPMRAlloc *)(this);
      // placement new the size in the second location
      new (newPtr + objectOffset)(std::size_t)(size);
      return ptrToReturn;
    } else {
      return nullptr;
    }
  }

  void freePtr(void *origPtr, size_t originalSize)
  {
    if (origPtr == nullptr) {
      return;
    }
    upstream->deallocate(static_cast<std::byte *>(origPtr) - memPadding, originalSize + memPadding);
  }

  void *Realloc(void *origPtr, size_t originalSize, size_t newSize)
  {
    if (newSize == 0) {
      freePtr(origPtr, originalSize);
      return nullptr;
    }

    if (newSize <= originalSize) {
      return origPtr;
    }

    void *newPtr = Malloc(newSize);
    std::memcpy(newPtr, origPtr, originalSize);
    freePtr(origPtr, originalSize);
    return newPtr;
  }

  // and Free needs to be static, which causes this whole thing
  // to fall apart. This means that we have to keep our own list of allocated memory
  // with our own pointers back to ourselves and our own list of sizes
  // so we can push all of this back to the upstream allocator
  static void Free(void *ptr)
  {
    if (ptr == nullptr ) {
      return;
    }

    std::byte *startOfData = static_cast<std::byte *>(ptr) - memPadding;

    auto *ptrToAllocator = *reinterpret_cast<RapidJSONPMRAlloc **>(startOfData);
    auto origAllocatedSize = *reinterpret_cast<std::size_t *>(startOfData + objectOffset);

    ptrToAllocator->freePtr(ptr, origAllocatedSize);
  }
};


void parseMe(volatile const char * s, size_t sz)
{
#if OPTIMIZED_HEAP
  std::pmr::monotonic_buffer_resource mr;
  std::pmr::polymorphic_allocator<> pa{ &mr };
  auto &p = *pa.new_object<boost::json::stream_parser>();
  p.reset(pa);
#endif
  boost::json::error_code ec;
  
#if 0
  boost::json::parser ps;
  ps.write(s, sz, ec);
#else
  boost::json::stream_parser ps;
  ps.write((const char *) s, sz, ec);
  if (!ec) {
    ps.finish(ec);
  }
#if !OPTIMIZED_HEAP
  if (!ec) {
#if !CHEAPEN
    auto jv = ps.release();
#endif
  }
#endif
#endif
}

int main()
{
  //   std::ifstream i("gsoc-2018.json");
  std::ifstream i("citm_catalog.json");
  std::ostringstream sstr;
  sstr << i.rdbuf();
  //  mallopt(M_MMAP_THRESHOLD, 10487560 + 32);
  auto str = sstr.str();
  auto data = str.data();
  volatile auto sz = str.size();
  char * buf = new char[3 * 1048576];
  for (auto i = 0; i < 1000; i++)
  {
#if CHEAPEN
    cheap::cheap<cheap::DISABLE_FREE | cheap::NONZERO | cheap::SINGLE_THREADED | cheap::FIXED_BUFFER> reg(8, buf, 3 * 1048576);
    // cheap::cheap<cheap::NONZERO | cheap::SINGLE_THREADED | cheap::DISABLE_FREE> reg;
#endif
    parseMe(data, sz);
  }
  return 0;
}
