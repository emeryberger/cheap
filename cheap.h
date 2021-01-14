#pragma once

#include <stdlib.h>
#include <malloc.h>

extern "C" {
  void region_begin(bool allAligned,
                  bool allNonZero, bool sizeTaken,
		  bool sameSize, bool disableFree,
		  size_t oneSize);
void region_end();
}

#if defined(__cplusplus)

#include "common.hpp"
#include "regionheap.h"
#include "nextheap.hpp"

using namespace HL;

// FIXME? use UniqueHeap...
class TopHeap : public SizeHeap<LockedHeap<SpinLock, ZoneHeap<MmapHeap, 65536>>> {};

class CheapHeapType :
  public KingsleyHeap<AdaptHeap<DLList, TopHeap>, TopHeap> {};

class CheapRegionHeap :
  public RegionHeap<CheapHeapType, 2, 1, 65536> {};


const char zone_adapt[] = "zone-adapt";
const char adapt_top[] = "adapt-top";
const char top[] = "top";

template <const char * name, typename SuperHeap>
class PrintMeHeap :  public SuperHeap {
public:
  void * malloc(size_t sz) {
    auto ptr = SuperHeap::malloc(sz);
    // tprintf::tprintf("@ malloc request (@) = @\n", name, sz, ptr);
    return ptr;
  }
};

class CheapFreelistHeap :
  public FreelistHeap<ZoneHeap<PrintMeHeap<zone_adapt, PrintMeHeap<adapt_top, PrintMeHeap<top, TopHeap>>>, 65536>> {};


namespace cheap {

enum flags {
  ALIGNED = 0b0000'0001,  // no need to align sizes
  NONZERO = 0b0000'0010,  // no zero size requests
  SIZE_TAKEN = 0b0000'0100, // need support for size
  SINGLE_THREADED = 0b0000'1000, // all requests the same size - use freelist
  DISABLE_FREE = 0b0001'0000, // frees -> NOPs: use a "region" allocator
  SAME_SIZE = 0b0010'0000, // all requests the same size
};


  class cheap_base {};

template <int Flags>
class cheap : public cheap_base {
public:
  inline cheap(size_t sz = 8) {
    static_assert(flags::ALIGNED ^ flags::NONZERO ^ flags::SIZE_TAKEN ^ flags::SINGLE_THREADED ^ flags::DISABLE_FREE ^ flags::SAME_SIZE == (1 << 7) - 1,
		  "Flags must be one bit and mutually exclusive.");
    region_begin(Flags & flags::ALIGNED, Flags & flags::NONZERO,
                 Flags & flags::SIZE_TAKEN, Flags & flags::SAME_SIZE, Flags & flags::DISABLE_FREE, sz);
  }
  inline ~cheap() {
    region_end();
  }
};


class cheap_info {
public:
  alignas(double) char cheap_buf[sizeof(cheap_base)];
  cheap_base * cheap;
  bool in_cheap{false};
  bool all_aligned{false}; // no need to align sizes
  bool all_nonzero{false}; // no zero size requests
  bool size_taken{true};   // need metadata for size
  bool disable_free{true}; // use a "region" allocator
  bool same_size{false};   // all requests the same size - use freelist
  size_t one_size{0};      // the one size (if above)
  CheapRegionHeap region;
  CheapFreelistHeap freelist;
};

class cheap_header {
public:
  cheap_header(size_t sz) : object_size(sz) {}
  alignas(max_align_t) size_t object_size;
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
