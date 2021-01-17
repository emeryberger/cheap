#pragma once

#define MIN_ALIGNMENT 8
//#define MIN_ALIGNMENT alignof(max_align_t)
#define THREAD_SAFE 1

#include <stdlib.h>
#include <malloc.h>

#if defined(__cplusplus)

#include "common.hpp"
#include "regionheap.h"
#include "nextheap.hpp"

using namespace HL;

class TopHeap : public SizeHeap<LockedHeap<SpinLock, ZoneHeap<SizedMmapHeap, 65536>>> {};

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

class CheapRegionHeap :
  public RegionHeap<ZoneHeap<SizedMmapHeap, 65536>, 2, 1, 3 * 1048576> {};

const char zone_adapt[] = "zone-adapt";
const char adapt_top[] = "adapt-top";
const char top[] = "top";

class CheapFreelistHeap :
  public FreelistHeap<ZoneHeap<SizedMmapHeap,
			       4096>> {};

namespace cheap {

  enum flags {
    ALIGNED = 0b0000'0001,  // no need to align sizes
    NONZERO = 0b0000'0010,  // no zero size requests
    SIZE_TAKEN = 0b0000'0100, // need support for size
    SINGLE_THREADED = 0b0000'1000, // all requests the same size - use freelist
    DISABLE_FREE = 0b0001'0000, // frees -> NOPs: use a "region" allocator
    SAME_SIZE = 0b0010'0000, // all requests the same size
    FIXED_BUFFER = 0b0100'0000, // use a specified buffer
  };

  class cheap_base;
}

extern cheap::cheap_base*& current();

namespace cheap {
  class cheap_base {
  public:
    virtual void * malloc(size_t) = 0;
    virtual void free(void *) = 0;
    virtual size_t getSize(void *) = 0;
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

  template <const int Flags>
  class cheap : public cheap_base {
  public:
    inline cheap(size_t sz = 8,
		 char * buf = nullptr,
		 size_t bufSz = 0)
    {
      static_assert(flags::ALIGNED ^ flags::NONZERO ^ flags::SIZE_TAKEN ^ flags::SINGLE_THREADED ^ flags::DISABLE_FREE ^ flags::SAME_SIZE ^ flags::FIXED_BUFFER == (1 << 8) - 1,
		    "Flags must be one bit and mutually exclusive.");
      _oneSize = sz;
      _buf = buf;
      _bufSz = bufSz;
      current() = this;
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
	  if (Flags & flags::FIXED_BUFFER) {
	    ptr = _buf;
	    _buf += sz;
	  } else {
	    ptr = region.malloc(sz);
	  }
	} else {
	  if (Flags & flags::FIXED_BUFFER) {
	    ptr = _buf;
	    _buf += sz;
	  } else {
	    ptr = region.malloc(sz + sizeof(cheap_header));
	  }
	  // Prepend an object header.
	  new (ptr) cheap_header(sz);
	}
      } else {
	assert(Flags & flags::SAME_SIZE);
	assert(sz == req_sz);
	ptr = freelist.malloc(sz);
#if 0
	if (!ptr) {
	  tprintf::tprintf("@, @\n", ptr, req_sz);
	  abort();
	}
#endif
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
    char * _buf;
    size_t _bufSz;
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
