#pragma once

#ifndef SIMPOOL_HPP
#define SIMPOOL_HPP

#include "dllist.h"

#include <cstddef>
#include <cstdlib>

class SimPool {
public:

  ~SimPool()
  {
    release();
  }
  
  inline void * malloc(size_t sz) {
    // Get an object of the requested size plus the header.
    auto * b = reinterpret_cast<HL::DLList::Entry *>(::malloc(sz + sizeof(HL::DLList::Entry)));
    // Add to the front of the linked list.
    _list.insert(b);
    // Return just past the header.
    return reinterpret_cast<void *>(b + 1);
  }

  inline void free(void * ptr) {
    // Find the actual block header.
    auto * b = reinterpret_cast<HL::DLList::Entry *>(ptr) - 1;
    // Remove from the linked list.
    _list.remove(b);
    // Free the object.
    ::free(b);
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
  }
  
private:

  HL::DLList _list;
  
};


#endif
