#pragma once

#ifndef SIMREGION_HPP
#define SIMREGION_HPP

#include <algorithm>
#include <vector>
#ifndef __APPLE__
#include <malloc.h>
#else
#include <malloc/malloc.h>
#endif

class SimRegion {
public:

  static constexpr int VECTOR_INITIAL_SIZE = 0;
  
  enum { Alignment = alignof(std::max_align_t) }; // guaranteed by malloc

  static inline constexpr size_t align(size_t sz) {
    return (sz + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
  }

  SimRegion(unsigned int size = VECTOR_INITIAL_SIZE)
    : _vectorInitialSize (size)
  {
    _allocated.reserve(_vectorInitialSize);
  }
  
  ~SimRegion() {
    release();
  }

  static inline size_t getSize(void * ptr) {
#ifndef __APPLE__
    return ::malloc_usable_size(ptr);
#else
    return ::malloc_size(ptr);
#endif
  }
  
  inline void * malloc(size_t sz) {
    if (sz == 0) {
      return nullptr;
    }
    //    sz = align(sz);
    auto ptr = ::malloc(sz);
    _allocated.push_back(ptr);
    return ptr;
  }

  inline void * memalign(size_t alignment, size_t size) {
    // Check for non power-of-two alignment.
    if ((alignment == 0) || (alignment & (alignment - 1)))
      {
	return nullptr;
      }
    
    if (alignment <= Alignment) {
      // Already aligned by default.
      return malloc(size);
    } else {
      // Try to just allocate an object of the requested size.
      // If it happens to be aligned properly, just return it.
      void * ptr = malloc(size);
      if (((size_t) ptr & ~(alignment - 1)) == (size_t) ptr) {
	// It is already aligned just fine; return it.
	return ptr;
      }
      // It was not aligned as requested: free the object and allocate a big one,
      // and align within.
      free(ptr);
      ptr = malloc (size + 2 * alignment);
      void * alignedPtr = (void *) (((size_t) ptr + alignment - 1) & ~(alignment - 1));
      return alignedPtr;
    }
  }
  
  inline void free(void *) {
  }

  inline void * allocate(size_t sz) {
    return malloc(sz);
  }

  inline void deallocate(void *)
  {
  }

  inline size_t size() {
    return _allocated.size();
  }
  
  void rewind()
  {
    std::for_each(_allocated.begin(), _allocated.end(), std::free);
    _allocated.clear();
  }

  void release()
  {
    rewind();
    _allocated.shrink_to_fit();
    _allocated.reserve(_vectorInitialSize);
  }
  
private:

  std::vector<void *> _allocated;
  size_t _vectorInitialSize;
};

#endif // SIMREGION_HPP
