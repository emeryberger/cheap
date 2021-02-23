#pragma once

#ifndef SHIM_ALLOCATOR_HPP
#define SHIM_ALLOCATOR_HPP

#include <cassert>
#include <cstddef>
#include <vector>
#include <iostream>

#define USE_DLLIST 1
#define COLLECT_STATS 0  // FIXME

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

class ShimAllocator : public bdlma::ManagedAllocator {
 public:

  ShimAllocator(bslma::Allocator *)
      : _allocations(0),
        _allocated(0),
        _frees(0),
        _allocList(&_allocList, &_allocList) {
#if !USE_DLLIST
    _allocVector.reserve(128 * 1024);
#endif
    //    std::cout << "SHIM" << std::endl;
  }

  virtual ~ShimAllocator() {
    release();
    printStats();
  }

  void printStats() {
#if !COLLECT_STATS
#else
    std::cout << _allocations << " allocations (" << _allocated
              << " allocated), " << _frees << " frees\n";
#endif
  }

  virtual void rewind() {
    // rewind is the equivalent of release here, since we do not maintain internal buffers.
    release();
  }

  virtual void release() {
#if USE_DLLIST
    // Iterate through the allocation list, freeing objects one at a time.
    header *p = _allocList.next;
    while (p != &_allocList) {
      header *q = p->next;
#if COLLECT_STATS
      _frees++;
#endif
      ::free(p);
      p = q;
    }
    // Reset the list to its original (empty) state.
    _allocList.prev = &_allocList;
    _allocList.next = &_allocList;
#else
    _frees += _allocVector.size();
    for (auto i : _allocVector) {
      ::free(i);
    }
    _allocVector.clear();
#endif
    // printStats();
  }

#warning "including the shim"
  
  inline virtual void *allocate(size_type sz) {
    if (sz == 0) {
      return nullptr;
    }
#if COLLECT_STATS
    _allocations++;
    if (_allocations % 100000 == 0) {
      printf("Shim allocation: %d (%ld)\n", _allocations, sz);
    }
    _allocated += sz;
#endif
#if USE_DLLIST
    // Allocate extra space for the header, which is used to thread
    // this object into the doubly-linked allocation list.
    void *ptr = ::malloc(sz + sizeof(header));
    header *h = new (ptr) header(&_allocList, _allocList.next);
    _allocList.next->prev = h;
    _allocList.next = h;
    // Return memory just past the start of the header.
    return static_cast<void *>(h + 1);
#else
    void * ptr = ::malloc(sz);
    _allocVector.push_back(ptr);
    return ptr;
#endif
  }

  inline virtual void deallocate(void *address) {
    if (address == nullptr) {
      return;
    }
#if COLLECT_STATS
    _frees++;
#endif
#if USE_DLLIST
    // Recover the header.
    header *h = reinterpret_cast<header *>(address) - 1;
    // Splice out of allocList.
    h->next->prev = h->prev;
    h->prev->next = h->next;
    ::free(h);
#else
    // FIXME - for now, no op
#endif
  }

  template <class TYPE>
  void deleteObject(const TYPE *object) {
    object->~TYPE();
    deallocate(object);
  }
  void deleteObject(bsl::nullptr_t ptr) {}

  template <class TYPE>
  void deleteObjectRaw(const TYPE *object) {
    object->~TYPE();
    deallocate(object);
  }
  void deleteObjectRaw(bsl::nullptr_t ptr) {}

 private:
  class header {
   public:
    header(header *prev_, header *next_) {
      prev = prev_;
      next = next_;
    }
    alignas(std::max_align_t) header *prev;
    header *next;
  };

#if !USE_DLLIST
  std::vector<void *> _allocVector;
#endif
  
  header _allocList;  // the doubly-linked list containing all allocated objects
  size_t _allocations;  // total number of allocations
  size_t _allocated;    // total bytes allocated
  size_t _frees;        // total number of frees
};

#endif  // SHIM_ALLOCATOR_HPP
