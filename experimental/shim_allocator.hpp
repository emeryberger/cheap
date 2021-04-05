#pragma once

#ifndef SHIM_ALLOCATOR_HPP
#define SHIM_ALLOCATOR_HPP

#include "tprintf.h"

#define USE_ORIGINAL 0

#if USE_ORIGINAL
#warning "original bufferedsequentialallocator"
#else
#warning "SHIM bufferedsequentialallocator"
#endif

#include <cassert>
#include <cstddef>
#include <vector>
#include <iostream>

#include <execinfo.h>

#define USE_DLLIST 0
#define COLLECT_STATS 0  // FIXME
#define REPORT_STATS 0

#include <bdlma_managedallocator.h>
#include <bdlma_bufferedsequentialpool.h>
#include <bdlma_managedallocator.h>

#include <bslma_allocator.h>

#include <bsls_alignment.h>
#include <bsls_blockgrowth.h>
#include <bsls_performancehint.h>
#include <bsls_types.h>

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

class ShimAllocator : public bslma::Allocator {
// bdlma::ManagedAllocator {
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
#if USE_DLLIST
    _allocList (new header)
#else
    _allocVector (new std::vector<void *>)
#endif
#if COLLECT_STATS
    ,
      _allocations(0),
    _allocated(0),
    _frees(0),
    _deallocations(0),
    _rewinds(0),
    _releases(0)
#endif
  {
#if !USE_DLLIST
    _allocVector->reserve(8); // 128); //  * 1024);
#endif
    //        std::cout << "SHIM" << std::endl;
  }

  virtual ~ShimAllocator() {
    //    tprintf::tprintf("SHIM DESTRUCTION\n");
#if 0
    if (_allocations == 0) {
      void * frames[16];
      auto numframes = backtrace(frames, 16);
      backtrace_symbols_fd(frames, numframes, fileno(stdout));
    }
#endif
    release();
    printStats();
#if USE_DLLIST
    delete _allocList;
#else
    delete _allocVector;
#endif
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
#if USE_DLLIST
    if (!_allocList) {
      return;
    }
    // Iterate through the allocation list, freeing objects one at a time.
    header *p = _allocList->begin();
    // tprintf::tprintf("p = @, &allocList = @\n", p, _allocList);
    while (p) {
      header *q = p->getNext();
#if COLLECT_STATS
      _frees++;
#endif
      //tprintf::tprintf("RELEASE @: ABOUT TO FREE @\n", this, p);
      ::free(p);
      p = q;
    }
    // Reset the list to its original (empty) state.
    _allocList->clear();
#else
#if COLLECT_STATS
    _frees += _allocVector->size();
#endif
    if (_allocVector) {
      for (auto p : *_allocVector) {
	//	tprintf::tprintf("RELEASE @: ABOUT TO FREE @\n", this, p);
	::free(p);
      }
      _allocVector->clear();
    }
#endif
    // printStats();
  }

#warning "including the shim"

  inline constexpr size_t align(size_t sz) {
    return (sz + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
  }
  
  inline void *allocate(size_type sz) {
    //    tprintf::tprintf("SHIM ALLOCATE: this = @\n", this);
    if (sz == 0) {
      return nullptr;
    }
    sz = align(sz);
#if COLLECT_STATS
    _allocations++;
#if 0
    if (_allocations % 100000 == 0) {
      printf("Shim allocation: %zu (%ld)\n", _allocations, sz);
    }
#endif
    _allocated += sz;
#endif
#if USE_DLLIST
    // Allocate extra space for the header, which is used to thread
    // this object into the doubly-linked allocation list.
    void *ptr = ::malloc(sz + sizeof(header));
    _allocList->push(ptr);
    // Return memory just past the start of the header.
    //    tprintf::tprintf("allocated @ for @ (really @), returning @\n", ptr, sz, sz + sizeof(header), h + 1);
    return static_cast<void *>((header *) ptr + 1);
#else
    void * ptr = ::malloc(sz);
    _allocVector->push_back(ptr);
    return ptr;
#endif
  }

  inline void * malloc(size_type sz) {
    return allocate(sz);
  }

  inline void free(void * ptr) {
    return;
    deallocate(ptr);
  }
  
  inline void deallocate(void *address) {
    return;
#if 0
    tprintf::tprintf("SHIM DEALLOCATE\n");
    if (address == nullptr) {
      return;
    }
#if COLLECT_STATS
    _deallocations++;
    _frees++;
#endif
#if USE_DLLIST
    // Recover the header.
    header *h = reinterpret_cast<header *>(address) - 1;
    // Splice out of allocList.
    // h->remove();
    tprintf::tprintf("deallocate about to free @\n", h);
    ::free(h);
#else
    // FIXME - for now, no op
#endif
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
#if USE_DLLIST
    if (_allocList) {
      ///      release();
    }
    _allocList = new header();
    //    _allocList = that._allocList; // new header();
#else
    _allocVector = new std::vector<void *>;
#endif
  }

  ShimAllocator(ShimAllocator&& that) noexcept {
#if USE_DLLIST
    that._allocList = new header();
#endif
  }
  
  ShimAllocator& operator=(const ShimAllocator& that) noexcept {
    return *this;
  }
  
  ShimAllocator& operator=(ShimAllocator&& that) noexcept {
    release();
#if USE_DLLIST
    _allocList = that._allocList;
    that._allocList = new header();
#endif
    return *this;
  }

 
 private:

  class header {
   public:
    header()
      : _next(nullptr)
    {}
    header(header *next_)
      : _next(next_)
    {
      // tprintf::tprintf("setting header prev=@, next=@\n", _prev, _next);
    }
    void clear() {
      _next = nullptr;
    }
    bool atEnd(header * p) const {
      return p == nullptr;
    }
    header * begin() {
      return _next;
    }
    header * getNext() {
      return _next;
    }
    header * push(void * ptr) {
      auto h = new (ptr) header(_next);
      _next = h;
      return h;
    }
  private:    
    void remove() {
    }
    alignas(std::max_align_t) header * _next;
  };

  friend bool operator==(const ShimAllocator& dis, const ShimAllocator& dat);
  friend bool operator!=(const ShimAllocator& dis, const ShimAllocator& dat);
  
#if !USE_DLLIST
  std::vector<void *> * _allocVector { nullptr };
#else
  header * _allocList { nullptr };  // the doubly-linked list containing all allocated objects
#endif
  
#if COLLECT_STATS
  size_t _allocations;  // total number of allocations
  size_t _allocated;    // total bytes allocated
  size_t _frees;        // total number of frees
  size_t _deallocations;
  size_t _releases;
  size_t _rewinds;
#endif
};

bool operator==(const ShimAllocator& dis, const ShimAllocator& dat) {
  printf("OPERATOR==\n");
#if USE_DLLIST
  return dis._allocList == dat._allocList;
#else
  return dis._allocVector == dat._allocVector;
#endif
}
  
bool operator!=(const ShimAllocator& dis, const ShimAllocator& dat) {
  printf("OPERATOR!=\n");
#if USE_DLLIST
  return dis._allocList != dat._allocList;
#else
  return dis._allocVector != dat._allocVector;
#endif
}
  

#endif  // SHIM_ALLOCATOR_HPP
