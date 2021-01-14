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

#define MIN_ALIGNMENT 8
//#define MIN_ALIGNMENT alignof(max_align_t)
#define THREAD_SAFE 1

#if THREAD_SAFE
static thread_local cheap::cheap_info info __attribute__((tls_model ("initial-exec")));
#else
static cheap::cheap_info info;
#endif

extern "C" ATTRIBUTE_EXPORT void region_begin(bool allAligned = false,
                                              bool allNonZero = false,
                                              bool sizeTaken = true,
					      bool sameSize = false,
					      bool disableFree = true,
					      size_t oneSize = 8) {
  auto &ci = info;
  ci.in_cheap = true;
  ci.all_aligned = allAligned;
  ci.all_nonzero = allNonZero;
  ci.size_taken = sizeTaken;
  ci.same_size = sameSize;
  ci.one_size = oneSize;
  ci.disable_free = disableFree;
}

extern "C" ATTRIBUTE_EXPORT void region_end() {
  auto &ci = info;
  ci.region.reset();
  ci.freelist.clear();
  ci.in_cheap = false;
}

extern "C" size_t __attribute__((always_inline)) xxmalloc_usable_size(void *ptr) {
  auto &ci = info;
  if (ci.in_cheap) {
    assert(ci.size_taken);
    if (ci.same_size) {
      return ci.one_size;
    } else {
      return ((cheap::cheap_header *)ptr - 1)->object_size;
    }
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
    void * ptr;
    if (ci.disable_free) {
      if (!ci.size_taken && !ci.same_size) {
	ptr = ci.region.malloc(req_sz);
      } else {
	ptr = ci.region.malloc(req_sz + sizeof(cheap::cheap_header));
	// Prepend an object header.
	new (ptr) cheap::cheap_header(req_sz);
      }
    } else {
      assert(ci.same_size);
      ptr = ci.freelist.malloc(req_sz);
      if (!ptr) {
	tprintf::tprintf("@, @\n", ptr, req_sz);
	abort();
      }
    }
    return ptr;
  }
  return getTheCustomHeap().malloc(sz);
}

extern "C" void __attribute__((always_inline)) xxfree(void *ptr) {
  if (!info.in_cheap) {
    getTheCustomHeap().free(ptr);
    return;
  }
  if (!info.disable_free) {
    assert(info.same_size);
    info.freelist.free(ptr);
  }
}

extern "C" void __attribute__((always_inline)) xxfree_sized(void *ptr, size_t) {
  xxfree(ptr);
}

extern "C" void * __attribute__((always_inline)) xxmemalign(size_t alignment, size_t sz) {
  auto &ci = info;
  if (ci.in_cheap) {
    // Round up the region pointer to the required alignment.
    // auto bufptr = reinterpret_cast<uintptr_t>(ci.region.malloc(sz));
    // FIXME THIS IS NOT ENOUGH
    //    ci.region_buffer =
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
