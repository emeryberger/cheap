#pragma once

#ifndef SHIM_ALLOCATOR_HPP
#define SHIM_ALLOCATOR_HPP

#include <cassert>
#include <cstddef>

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

class ShimAllocator : public BloombergLP::bdlma::ManagedAllocator {
 public:
  ShimAllocator(BloombergLP::bslma::Allocator *)
      : _allocations(0),
        _allocated(0),
        _frees(0),
        _allocList(&_allocList, &_allocList) {}

  ~ShimAllocator() {
    release();
    // printStats();
  }

  void printStats() {
    std::cout << _allocations << " allocations (" << _allocated
              << " allocated), " << _frees << " frees\n";
  }

  void rewind() = delete;

  void release() {
    // Iterate through the allocation list, freeing objects one at a time.
    header *p = _allocList.next;
    while (p != &_allocList) {
      header *q = p->next;
      _frees++;
      ::free(p);
      p = q;
    }
    // Reset the list to its original (empty) state.
    _allocList.prev = &_allocList;
    _allocList.next = &_allocList;
  }

  void *allocate(size_type sz) {
    assert(sz > 0);
    _allocations++;
    _allocated += sz;
    // Allocate extra space for the header, which is used to thread
    // this object into the doubly-linked allocation list.
    void *ptr = ::malloc(sz + sizeof(header));
    header *h = new (ptr) header(&_allocList, _allocList.next);
    _allocList.next->prev = h;
    _allocList.next = h;
    // Return memory just past the start of the header.
    return static_cast<void *>(h + 1);
  }

  void deallocate(void *address) {
    _frees++;
    // Recover the header.
    header *h = reinterpret_cast<header *>(address) - 1;
    // Splice out of allocList.
    h->next->prev = h->prev;
    h->prev->next = h->next;
    ::free(h);
  }

  template <class TYPE>
  void deleteObject(const TYPE *object) {
    delete object;
  }
  void deleteObject(bsl::nullptr_t ptr) { ::free(ptr); }

  template <class TYPE>
  void deleteObjectRaw(const TYPE *object) {
    delete object;
  }
  void deleteObjectRaw(bsl::nullptr_t ptr) { ::free(ptr); }

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

  header _allocList;  // the doubly-linked list containing all allocated objects
  size_t _allocations;  // total number of allocations
  size_t _allocated;    // total bytes allocated
  size_t _frees;        // total number of frees
};

#endif  // SHIM_ALLOCATOR_HPP