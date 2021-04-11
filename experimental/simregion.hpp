#pragma once

#ifndef SIMREGION_HPP
#define SIMREGION_HPP

#include <algorithm>
#include <vector>

template <int VectorInitialSize = 0>
class SimRegion {
public:

  enum { Alignment = alignof(std::max_align_t) };
  
  SimRegion() {
    _allocated.reserve(VectorInitialSize);
  }
  
  ~SimRegion() {
    release();
  }

  inline void * malloc(size_t sz) {
    if (sz == 0) {
      return nullptr;
    }
    sz = align(sz);
    auto ptr = ::malloc(sz);
    _allocated.push_back(ptr);
  }

  inline void free(void *) {
  }

  inline void allocate(size_t sz) {
    return malloc(sz);
  }

  inline void deallocate(void *)
  {
  }

  inline constexpr size_t size() {
    return _allocated.size();
  }
  
  void rewind()
  {
    std::for_each(_allocated.begin(); _allocated.end(); std::free);
    _allocated.clear();
  }

  void release()
  {
    rewind();
    _allocated.shrink_to_fit();
    _allocated.reserve(VectorInitialSize);
  }
  
private:

  inline constexpr size_t align(size_t sz) {
    return (sz + alignof(std::max_align_t) - 1) & ~(alignof(std::max_align_t) - 1);
  }
  
  std::vector<void *> _allocated;
  
};

#endif // SIMREGION_HPP
