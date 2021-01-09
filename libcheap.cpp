/*
  libcheap.cpp
     enables easy use of regions
   invoke `region_begin(buf, sz)` --> all subsequent `malloc`s use the buffer,
  `free`s are ignored
   invoke `region_end()` --> back to normal `malloc`/`free` behavior

   see the C++ API in cheap.h

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

class cheap_info {
public:
  bool in_region{false};
  char *region_buffer{nullptr};
  size_t region_size_remaining{0};
  bool all_aligned{false}; //  if true, no need to align sizes
  bool all_nonzero{false}; //  if true, no zero size requests
  bool size_taken{true};   // if true, need metadata for size
};

static thread_local cheap_info info;

extern "C" ATTRIBUTE_EXPORT void region_begin(void *buf, size_t sz,
                                              bool allAligned = false,
                                              bool allNonZero = false,
                                              bool sizeTaken = true) {
  auto &ci = info;
  ci.region_buffer = reinterpret_cast<char *>(buf);
  ci.region_size_remaining = sz;
  ci.in_region = true;
  ci.all_aligned = allAligned;
  ci.all_nonzero = allNonZero;
  ci.size_taken = sizeTaken;
  if (ci.size_taken) {
    // Can't currently use regions if size was taken.
    ci.in_region = false;
  }
}

extern "C" ATTRIBUTE_EXPORT void region_end() {
  auto &ci = info;
  ci.in_region = false;
  ci.region_buffer = nullptr;
  ci.region_size_remaining = 0;
}

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void *ptr) {
  return getTheCustomHeap().getSize(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmalloc(size_t sz) {
  auto &ci = info;
  if (ci.in_region) {
    // Enforce default alignment.
    if (!ci.all_aligned) {
      if (!ci.all_nonzero) {
        if (sz < alignof(max_align_t)) {
          sz = alignof(max_align_t);
        }
      }
      sz = (sz + alignof(max_align_t) - 1) & ~(alignof(max_align_t) - 1);
    }
    if (ci.region_size_remaining < sz) {
      return nullptr;
    }
    auto oldbuf = ci.region_buffer;
    ci.region_buffer += sz;
    ci.region_size_remaining -= sz;
    return oldbuf;
  }
  return getTheCustomHeap().malloc(sz);
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void *ptr) {
  if (!info.in_region) {
    getTheCustomHeap().free(ptr);
  }
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void *ptr, size_t sz) {
  xxfree(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmemalign(size_t alignment, size_t sz) {
  auto &ci = info;
  if (ci.in_region) {
    // Round up the region pointer to the required alignment.
    auto bufptr = reinterpret_cast<uintptr_t>(ci.region_buffer);
    ci.region_buffer =
        reinterpret_cast<char *>((bufptr + alignment - 1) & ~(alignment - 1));
    return xxmalloc(sz);
  }
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}
