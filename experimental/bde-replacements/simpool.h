#pragma once

#ifndef SIMPOOL_HPP
#define SIMPOOL_HPP

#include "dllist.h"

#warning "SIMPOOL"

#include <cstddef>
#include <cstdlib>
#include <iostream>


#if defined(__APPLE__)
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

class SimPool {
public:

  static inline constexpr size_t align(size_t sz) {
    return (sz + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
  }

  SimPool(uint64_t maxSize = 0)
    : _maxSize (maxSize),
      _size (0)
  {
    //    static bool v = printInfo();
  }

  void resetSize(uint64_t maxSize) {
    _maxSize = maxSize;
    _size = 0;
  }
  
  bool printInfo() {
    std::cout << "SimPool initialized." << std::endl;
    return true;
  }
  
  ~SimPool()
  {
    release();
  }
  
  inline void * malloc(size_t sz) {
    if ((_maxSize) && (sz + _size > _maxSize)) {
      return nullptr;
    }
    // sz = align(sz);
    // Get an object of the requested size plus the header.
    auto * b = reinterpret_cast<DLList::Entry *>(::malloc(sz + sizeof(DLList::Entry)));
    if (b) {
      // Update current size of the pool.
      if (_maxSize) {
	_size += getSize(b);
      }
      // Add to the front of the linked list.
      _list.insert(b);
      // Return just past the header.
      return reinterpret_cast<void *>(b + 1);
    } else {
      return nullptr;
    }
  }

  inline void * allocate(size_t sz) {
    return malloc(sz);
  }
  
  inline void free(void * ptr) {
    // Find the actual block header.
    auto * b = reinterpret_cast<DLList::Entry *>(ptr) - 1;
    // Remove from the linked list.
    _list.remove(b);
    if (_maxSize) {
      _size -= getSize(b);
    }
    // Free the object.
    ::free(b);
  }

  inline void deallocate(void * ptr) {
    free(ptr);
  }
  
  bool isEmpty() {
    return _list.isEmpty();
  }
  
  void release() {
    // Iterate through the list, freeing each item.
    auto * e = _list.get();
    while (e) {
      ::free(e);
      e = _list.get();
    }
    _size = 0;
  }

  void rewind() {
    release();
  }
  
private:

  uint64_t getSize(void * ptr) {
#if defined(__APPLE__)
    auto sz = malloc_size(ptr);
#else
    auto sz = malloc_usable_size(ptr);
#endif
    return sz;
  }
  
  uint64_t _maxSize;
  uint64_t _size;
  DLList _list;
  
};


#endif
