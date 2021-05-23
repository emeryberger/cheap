#pragma once

#ifndef SHIM_ALLOCATOR_HPP
#define SHIM_ALLOCATOR_HPP

#include "tprintf.h"

#include <cassert>
#include <cstddef>
#include <vector>
#include <iostream>

#include <execinfo.h>

#define USE_DLLIST 0
#define COLLECT_STATS 0  // FIXME
#define REPORT_STATS 0

// #include <bdlma_bufferedsequentialpool.h>

#include <bslma_allocator.h>
#include <bslma_managedallocator.h>

#include <bsls_alignment.h>
#include <bsls_blockgrowth.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

#include "simregion.hpp"

using namespace BloombergLP;

/**
   ShimAllocator : adapter for Bloomberg allocators
   (https://github.com/bloomberg/bde) that routes all allocations to
   malloc/free. Used for experimental purposes (to empirically measure
   the benefit of custom (a.k.a. "local") allocators vs. malloc/free).

   * All allocations are threaded into a doubly-linked list, which is
   * freed _en masse_ when the allocator is destroyed or when release
   * is called. Individual objects can also be deleted individually:
   * these are then removed from the doubly-linked list.

 */

namespace BloombergLP {
namespace bslma {

class ShimAllocator : public bslma::ManagedAllocator {
  // class ShimAllocator : public bslma::Allocator {
 public:

  // Constructors to match BufferedSequentialAllocator constructors.
  // (We discard everything)

  typedef std::false_type propagate_on_container_copy_assignment;
  typedef std::true_type propagate_on_container_move_assignment;
  typedef std::true_type propagate_on_container_swap;
  typedef std::false_type is_always_equal;

  ShimAllocator(char *,
		bsls::Types::size_type size,
		bslma::Allocator * basicAllocator = 0)
    : ShimAllocator(basicAllocator)
  {}
  
  ShimAllocator(char                        *buffer,
		bsls::Types::size_type       size,
		bsls::BlockGrowth::Strategy  growthStrategy,
		bslma::Allocator            *basicAllocator = 0)
    : ShimAllocator(basicAllocator)
  {}
  
  ShimAllocator(char                      *buffer,
		bsls::Types::size_type     size,
		bsls::Alignment::Strategy  alignmentStrategy,
		bslma::Allocator          *basicAllocator = 0)
    : ShimAllocator(basicAllocator)
  {}
  
  ShimAllocator(char                        *buffer,
		bsls::Types::size_type       size,
		bsls::BlockGrowth::Strategy  growthStrategy,
		bsls::Alignment::Strategy    alignmentStrategy,
		bslma::Allocator            *basicAllocator = 0)
    : ShimAllocator(basicAllocator)
  {}

  ShimAllocator(bslma::Allocator * = 0)
    :
    _allocVector (new SimRegion())
  {
  }

  virtual ~ShimAllocator() {
    release();
    printStats();
    delete _allocVector;
  }

  void printStats() {
#if REPORT_STATS
#if !COLLECT_STATS
#else
    std::cout << "Statistics for ShimAllocator " << this << std::endl;
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "allocations:\t" << _allocations << std::endl;
    std::cout << "alloc-bytes:\t" << _allocated << std::endl;
    std::cout << "deallocations:\t" << _deallocations << std::endl;
    std::cout << "frees:        \t" << _frees << std::endl;
    std::cout << "rewinds:\t" << _rewinds << std::endl;
    std::cout << "releases:\t" << _releases << std::endl;
#endif
#endif
  }

  virtual void rewind() {
    // rewind is the equivalent of release here, since we do not maintain internal buffers.
#if COLLECT_STATS
    _rewinds++;
#endif
    release();
  }

  virtual void release() {
    // tprintf::tprintf("RELEASE : this = @\n", this);
#if COLLECT_STATS
    _releases++;
#endif
#if COLLECT_STATS
    _frees += _allocVector->size();
#endif
    if (_allocVector) {
      _allocVector->release();
    }
    // printStats();
  }

#warning "including the shim"

  inline void *allocateAndExpand(int * size)
  {
    if (*size == 0) {
      return nullptr;
    }
    auto ptr = allocate(*size);
    *size = SimRegion::getSize(ptr);
    return ptr;
  }

  inline void * allocateAndExpand(int * sz,
				  int maxNumBytes)
  {
    // Undefined behavior if align(sz) > maxNumBytes
    return allocateAndExpand(sz);
  }

  inline int expand(void * address,
		    int originalNumBytes)
  {
    // Return the max amount already available.
    return SimRegion::getSize(address); // align(originalNumBytes);
  }

  inline int expand(void * address,
		    int originalNumBytes,
		    int maxNumBytes)
  {
    return expand(address, originalNumBytes);
  }

  virtual void reserveCapacity(int numBytes) {
  }

  inline int truncate(void * address,
		      int originalNumBytes,
		      int newNumBytes)
  {
    // Do nothing with the object's size - just return the original size.
    return SimRegion::getSize(address);
  }
  
  inline void *allocate(size_type sz) {
#if COLLECT_STATS
    _allocations++;
#if 0
    if (_allocations % 100000 == 0) {
      printf("Shim allocation: %zu (%ld)\n", _allocations, sz);
    }
#endif
    _allocated += sz;
#endif
    return _allocVector->allocate(sz);
  }

  inline void * malloc(size_type sz) {
    return allocate(sz);
  }

  inline void free(void * ptr) {
    return;
    // deallocate(ptr);
  }
  
  inline void deallocate(void *address) {
#if COLLECT_STATS
    _deallocations++;
    _frees++;
#endif
  }

  template <class TYPE>
  void deleteObject(const TYPE *object) {
#if 0
    object->~TYPE();
    deallocate(object);
#endif
  }
  void deleteObject(bsl::nullptr_t ptr) {}

  template <class TYPE>
  void deleteObjectRaw(const TYPE *object) {
#if 0
    object->~TYPE();
    deallocate(object);
#endif
  }
  void deleteObjectRaw(bsl::nullptr_t ptr) {}

  ShimAllocator(const ShimAllocator& that) noexcept {
    _allocVector = new SimRegion();
  }

  ShimAllocator(ShimAllocator&& that) noexcept {
  }
  
#if 1
  ShimAllocator& operator=(const ShimAllocator& that) noexcept {
    return *this;
  }
#endif
  
  ShimAllocator& operator=(ShimAllocator&& that) noexcept {
    that.release();
    that._allocVector = new SimRegion();
    // release();
    return *this;
  }

 
 private:

  inline friend bool operator==(const ShimAllocator& dis, const ShimAllocator& dat) {
    return dis._allocVector == dat._allocVector;
  }
  inline friend bool operator!=(const ShimAllocator& dis, const ShimAllocator& dat) {
    return dis._allocVector != dat._allocVector;
  }

  static constexpr int SIZE = 32; // 64;
  SimRegion * _allocVector { nullptr };
  
#if COLLECT_STATS
  size_t _allocations {0};  // total number of allocations
  size_t _allocated {0};    // total bytes allocated
  size_t _frees {0};        // total number of frees
  size_t _deallocations {0};
  size_t _releases {0};
  size_t _rewinds {0};
#endif
};

  #if 0
  
inline bool operator==(const ShimAllocator& dis, const ShimAllocator& dat) {
  return dis._allocVector == dat._allocVector;
}
  
inline bool operator!=(const ShimAllocator& dis, const ShimAllocator& dat) {
  return dis._allocVector != dat._allocVector;
}

  #endif

}
}

#endif  // SHIM_ALLOCATOR_HPP
