/*
  libcheapen.cpp
     enables easy use of regions
   invoke `region_begin(buf, sz)` --> all subsequent `malloc`s use the buffer, `free`s are ignored
   invoke `region_end()` --> back to normal `malloc`/`free` behavior
*/

// currently experimental only!

#include <heaplayers.h>

#include "common.hpp"
#include "nextheap.hpp"

#if defined(__APPLE__)
#include "macinterpose.h"
#endif

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

class ParentHeap : public NextHeap {};

class CustomHeapType : public ParentHeap {
public:
  void lock() {}
  void unlock() {}
};

CustomHeapType &getTheCustomHeap() {
  static CustomHeapType thang;
  return thang;
}

static thread_local bool in_region = false;
static thread_local char * region_buffer = nullptr;
static thread_local size_t region_size_remaining = 0;

extern "C" ATTRIBUTE_EXPORT void region_begin(void * buf, size_t sz) {
  region_buffer = reinterpret_cast<char *>(buf);
  region_size_remaining = sz;
  in_region = true;
}

extern "C" ATTRIBUTE_EXPORT void region_end() {
  in_region = false;
  region_buffer = nullptr;
  region_size_remaining = 0;
}

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void *ptr) {
  // Technically unsafe in region mode...FIX ME
  return getTheCustomHeap().getSize(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmalloc(size_t sz) {
  if (in_region) {
    if (region_size_remaining < sz) {
      return nullptr;
    }
    auto oldbuf = region_buffer;
    region_buffer += sz;
    region_size_remaining -= sz;
    return oldbuf;
  }
  return getTheCustomHeap().malloc(sz);
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void *ptr) {
  if (!in_region) {
    getTheCustomHeap().free(ptr);
  }
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void *ptr, size_t sz) {
  xxfree(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmemalign(size_t alignment, size_t sz) {
  if (in_region) {
    // FIXME: not yet implemented.
    return xxmalloc(sz);
  }
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}