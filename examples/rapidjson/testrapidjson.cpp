#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <memory_resource>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

#if CHEAPEN
#include "cheap.h"
#endif

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

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

void parseMe(const char * s, size_t sz)
{
  {
#if CHEAPEN
    cheap::cheap<cheap::ALIGNED | cheap::NONZERO | cheap::SINGLE_THREADED | cheap::SIZE_TAKEN | cheap::DISABLE_FREE> reg; //  | cheap::SIZE_TAKEN
#endif
    using namespace rapidjson;
#if 0
    CrtAllocator alloc;
    GenericDocument<UTF8<>, CrtAllocator> d(&alloc);
#else
    using namespace rapidjson;
    RapidJSONPMRAlloc alloc;
    GenericDocument<UTF8<>, RapidJSONPMRAlloc> d(&alloc);
    //Document d;
    // GenericDocument<UTF8<>> d;
#endif
    d.Parse(s, sz);
  }
}

int main()
{
  //   std::ifstream i("gsoc-2018.json");
  std::ifstream i("citm_catalog.json");
  std::ostringstream sstr;
  sstr << i.rdbuf();
  //  mallopt(M_MMAP_THRESHOLD, 10487560 + 32);
  for (auto i = 0; i < 1000; i++)
  {
    parseMe(sstr.str().data(), sstr.str().size());
  }
  return 0;
}
