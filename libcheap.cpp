/*
  libcheap.cpp
     enables easy use of regions
   invoke `region_begin(...)` --> all subsequent `malloc`s use the buffer,
  `free`s are ignored
   invoke `region_end()` --> back to normal `malloc`/`free` behavior

   see the C++ API in cheap.h

*/

#include <heaplayers.h>

#include "cheap.h"

#if defined(__APPLE__)
#include "macinterpose.h"
#endif

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

class cheap_current {
private:
  cheap_current();
public:
  cheap_current(const cheap_current&) = delete;
  cheap_current& operator=(const cheap_current&) = delete;
  cheap_current(cheap_current &&) = delete;
  cheap_current & operator=(cheap_current &&) = delete;
  inline static auto*& current() {
#if THREAD_SAFE
    static __thread cheap::cheap_base * c __attribute__((tls_model ("initial-exec")));
#else
    static cheap::cheap_base * c;
#endif
    return c;
  }
};

__attribute__((visibility("default"))) cheap::cheap_base*& current() {
  return cheap_current::current();
}

#if 1
#define FLATTEN __attribute__((flatten))
#else
#define FLATTEN
#endif

extern "C" size_t FLATTEN xxmalloc_usable_size(void *ptr) {
  auto ci = current();
  if (ci && ci->in_cheap) {
    return ci->getSize(ptr);
  }
  return getTheCustomHeap().getSize(ptr);
}

extern "C" void * FLATTEN xxmalloc(size_t req_sz) {
  size_t sz = req_sz;
  auto ci = current();
  //  tprintf::tprintf("xxmalloc(@) OH YEAH @\n", sz, ci);
  if (ci && ci->in_cheap) {
    return ci->malloc(sz);
  }
  return getTheCustomHeap().malloc(sz);
}

extern "C" void FLATTEN xxfree(void *ptr) {
  auto ci = current();
  if (!ci || !ci->in_cheap) {
    getTheCustomHeap().free(ptr);
    return;
  }
  return ci->free(ptr);
}

extern "C" void FLATTEN xxfree_sized(void *ptr, size_t) {
  xxfree(ptr);
}

extern "C" void * FLATTEN xxmemalign(size_t alignment, size_t sz) {
  auto ci = current();
  if (ci && ci->in_cheap) {
    // Round up the region pointer to the required alignment.
    // auto bufptr = reinterpret_cast<uintptr_t>(ci->region.malloc(sz));
    // FIXME THIS IS NOT ENOUGH
    //    ci->region_buffer =
    //    reinterpret_cast<char *>((bufptr + alignment - 1) & ~(alignment - 1));
    return xxmalloc(sz);
  }
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" void __attribute__((always_inline)) xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" void __attribute__((always_inline)) xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}

#include "gnuwrapper.cpp"
