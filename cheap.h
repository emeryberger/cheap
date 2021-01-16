#pragma once

#define MIN_ALIGNMENT 8
//#define MIN_ALIGNMENT alignof(max_align_t)
#define THREAD_SAFE 0

#include <stdlib.h>
#include <malloc.h>

#if defined(__cplusplus)

#include "common.hpp"
#include "regionheap.h"
#include "nextheap.hpp"

using namespace HL;

template <typename SuperHeap>
class StopMe : public SuperHeap {
public:
  void * malloc(size_t sz) {
    //    tprintf::tprintf("sz = @\n", sz);
    if (sz == 16384) {
      abort();
    }
    auto ptr = SuperHeap::malloc(sz);
    return ptr;
  }
};

// FIXME? use UniqueHeap...
class TopHeap : public SizeHeap<LockedHeap<SpinLock, ZoneHeap<StopMe<MmapHeap>, 65536>>> {};

class CheapHeapType :
  public KingsleyHeap<AdaptHeap<DLList, TopHeap>, TopHeap> {};

template <const char * name, typename SuperHeap>
class PrintMeHeap :  public SuperHeap {
public:
  inline void * malloc(size_t sz) {
    auto ptr = SuperHeap::malloc(sz);
    // tprintf::tprintf("@ malloc request (@) = @\n", name, sz, ptr);
    return ptr;
  }
};

const char region_cheap_heap[] = "region-cheap";

class CheapRegionHeap {
  //  public RegionHeap<PrintMeHeap<region_cheap_heap, CheapHeapType>, 2, 1, 65536>
public:
  CheapRegionHeap()
    : orig ((char *) h.map(10 * 1048576)),
      buf (orig),
      bufptr (buf)
  {}
  ~CheapRegionHeap() {
    h.unmap(buf, 10 * 1048576);
  }
  void * malloc(size_t sz) {
    auto ptr = bufptr;
    bufptr += sz;
    //    tprintf::tprintf("sz = @\n", sz);
    return ptr;
  }
  void free(void *){}
private:
  MmapWrapper h;
  char * orig;
  char * buf;
  char * bufptr;
};

const char zone_adapt[] = "zone-adapt";
const char adapt_top[] = "adapt-top";
const char top[] = "top";

class CheapFreelistHeap {
public:
  void * malloc(size_t);
  void free(void *);
};

#if 0
  public FreelistHeap<ZoneHeap<PrintMeHeap<zone_adapt,
					   PrintMeHeap<adapt_top,
						       PrintMeHeap<top,
								   CheapHeapType>>>,
			       128000>> {};
#endif

namespace cheap {

  enum flags {
    ALIGNED = 0b0000'0001,  // no need to align sizes
    NONZERO = 0b0000'0010,  // no zero size requests
    SIZE_TAKEN = 0b0000'0100, // need support for size
    SINGLE_THREADED = 0b0000'1000, // all requests the same size - use freelist
    DISABLE_FREE = 0b0001'0000, // frees -> NOPs: use a "region" allocator
    SAME_SIZE = 0b0010'0000, // all requests the same size
  };

  class cheap_base;
}

extern cheap::cheap_base*& current();

namespace cheap {
  class cheap_base {
  public:
    virtual void * malloc(size_t) { return nullptr; }
    virtual void free(void *) {}
    virtual size_t getSize(void *) { return 0; }
    //    virtual void * memalign(size_t, size_t) = 0;
    bool in_cheap {false};
  };
}

namespace cheap {
  class cheap_header {
  public:
    cheap_header(size_t sz) : object_size(sz) {}
    alignas(max_align_t) size_t object_size;
  };

  template <int Flags>
  class cheap : public cheap_base {
  public:
    inline cheap(size_t sz = 8) {
      static_assert(flags::ALIGNED ^ flags::NONZERO ^ flags::SIZE_TAKEN ^ flags::SINGLE_THREADED ^ flags::DISABLE_FREE ^ flags::SAME_SIZE == (1 << 7) - 1,
		    "Flags must be one bit and mutually exclusive.");
      _oneSize = sz;
      current() = this;
      // tprintf::tprintf("setem up @\n", current());
      in_cheap = true;
    }
    inline __attribute__((always_inline)) void * malloc(size_t req_sz) {
      assert(in_cheap);
      size_t sz = req_sz;
      if (!(Flags & flags::ALIGNED)) {
	// Enforce default alignment.
	if (!(Flags & flags::NONZERO)) {
	  // Ensure zero requests are rounded up.
	  if (sz < MIN_ALIGNMENT) {
	    sz = MIN_ALIGNMENT;
	  }
	}
	sz = (sz + MIN_ALIGNMENT - 1) & ~(MIN_ALIGNMENT - 1);
      }
      void * ptr;
      if (Flags & flags::DISABLE_FREE) {
	if (!((Flags & flags::SIZE_TAKEN) || (Flags & flags::SAME_SIZE))) {
	  ptr = region.malloc(req_sz);
	} else {
	  ptr = region.malloc(req_sz + sizeof(cheap_header));
	  // Prepend an object header.
	  new (ptr) cheap_header(req_sz);
	}
      } else {
	assert(Flags & flags::SAME_SIZE);
	ptr = freelist.malloc(req_sz);
	if (!ptr) {
	  tprintf::tprintf("@, @\n", ptr, req_sz);
	  abort();
	}
      }
      return ptr;
    }
    inline void free(void * ptr) {
      //      tprintf::tprintf("current now = @\n", current());
      assert(in_cheap);
      if (!(Flags & flags::DISABLE_FREE)) {
	assert(Flags & flags::SAME_SIZE);
	freelist.free(ptr);
      }
    }
    inline size_t getSize(void * ptr) {
      assert(Flags & flags::SIZE_TAKEN);
      if (Flags & flags::SAME_SIZE) {
	return _oneSize;
      } else {
	return ((cheap_header *)ptr - 1)->object_size;
      }
    }
#if 0
    inline void * memalign(size_t, size_t) {
    }
#endif
    inline ~cheap() {
      //      region.reset();
      //      freelist.clear(); // FIXME MAYBE
      in_cheap = false;
    }
  private:
    size_t _oneSize {0};
    CheapRegionHeap region;
    CheapFreelistHeap freelist;
  };

 
} // namespace cheap


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

class spin_lock {
public:
  void lock() {
    while (_lock.test_and_set(std::memory_order_acquire))
      ;
  }
  void unlock()
  {
    _lock.clear(std::memory_order_release);
  }
private:
  std::atomic_flag _lock = ATOMIC_FLAG_INIT;
};

#endif
