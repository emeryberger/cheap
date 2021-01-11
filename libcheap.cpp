/*
  libcheap.cpp
     enables easy use of regions
   invoke `region_begin(buf, sz)` --> all subsequent `malloc`s use the buffer,
  `free`s are ignored
   invoke `region_end()` --> back to normal `malloc`/`free` behavior

   see the C++ API in cheap.h

*/

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

#define TRACK_LAST_SIZE 0
//#define MIN_ALIGNMENT 8
#define MIN_ALIGNMENT alignof(max_align_t)
#define THREAD_SAFE 1

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
  bool in_cheap{false};
  char *region_buffer{nullptr};
  size_t region_size_remaining{0};
  bool all_aligned{false}; //  if true, no need to align sizes
  bool all_nonzero{false}; //  if true, no zero size requests
  bool size_taken{true};   // if true, need metadata for size
  // TBD: single size (so can return same size all the time)
#if TRACK_LAST_SIZE
  void *last_malloc{nullptr};
  size_t last_size{0};
#endif
};

class cheap_header {
public:
  cheap_header(size_t sz) : object_size(sz) {}
  alignas(max_align_t) size_t object_size;
};

#if THREAD_SAFE
static thread_local cheap_info info;
#else
static cheap_info info;
#endif

extern "C" ATTRIBUTE_EXPORT void region_begin(void *buf, size_t sz,
                                              bool allAligned = false,
                                              bool allNonZero = false,
                                              bool sizeTaken = true) {
  auto &ci = info;
  ci.region_buffer = reinterpret_cast<char *>(buf);
  ci.region_size_remaining = sz;
  ci.in_cheap = true;
  ci.all_aligned = allAligned;
  ci.all_nonzero = allNonZero;
  ci.size_taken = sizeTaken;
}

extern "C" ATTRIBUTE_EXPORT void region_end() {
  auto &ci = info;
  ci.in_cheap = false;
  ci.region_buffer = nullptr;
  ci.region_size_remaining = 0;
}

extern "C" size_t __attribute__((always_inline)) xxmalloc_usable_size(void *ptr) {
  auto &ci = info;
  if (ci.in_cheap) {
    assert(ci.size_taken);
    return ((cheap_header *)ptr - 1)->object_size;
  }
  return getTheCustomHeap().getSize(ptr);
}

extern "C" void * __attribute__((always_inline)) xxmalloc(size_t req_sz) {
  auto &ci = info;
  size_t sz = req_sz;
  if (ci.in_cheap) {
    if (!ci.all_aligned) {
      // Enforce default alignment.
      if (!ci.all_nonzero) {
        // Ensure zero requests are rounded up.
        if (sz < MIN_ALIGNMENT) {
          sz = MIN_ALIGNMENT;
        }
      }
      sz = (sz + MIN_ALIGNMENT - 1) & ~(MIN_ALIGNMENT - 1);
    }
    auto oldbuf = ci.region_buffer;
    if (ci.size_taken) {
      // Prepend an object header.
      new (oldbuf) cheap_header(req_sz);
      sz += sizeof(cheap_header);
      oldbuf += sizeof(cheap_header);
    }
    if (ci.region_size_remaining < sz) {
      return nullptr;
    }
    ci.region_buffer += sz;
    ci.region_size_remaining -= sz;
#if TRACK_LAST_SIZE
    ci.last_malloc = oldbuf;
    ci.last_size = sz;
#endif
    return oldbuf;
  }
  return getTheCustomHeap().malloc(sz);
}

extern "C" void __attribute__((always_inline)) xxfree(void *ptr) {
  if (!info.in_cheap) {
    getTheCustomHeap().free(ptr);
  }
}

extern "C" void __attribute__((always_inline)) xxfree_sized(void *ptr, size_t) {
  xxfree(ptr);
}

extern "C" void * __attribute__((always_inline)) xxmemalign(size_t alignment, size_t sz) {
  auto &ci = info;
  if (ci.in_cheap) {
    // Round up the region pointer to the required alignment.
    auto bufptr = reinterpret_cast<uintptr_t>(ci.region_buffer);
    ci.region_buffer =
        reinterpret_cast<char *>((bufptr + alignment - 1) & ~(alignment - 1));
    return xxmalloc(sz);
  }
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" void __attribute__((always_inline)) xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" void __attribute__((always_inline)) xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}

#include "gnuwrapper.cpp"
