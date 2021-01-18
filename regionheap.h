/* -*- C++ -*- */

#pragma once

#ifndef REGIONHEAP_H
#define REGIONHEAP_H

#include "heaplayers.h"

#include <assert.h>

template <class SuperHeap,
	  unsigned int MultiplierNumerator = 2,
	  unsigned int MultiplierDenominator = 1,
	  size_t ChunkSize = 4096>
class RegionHeap : public SuperHeap {
public:

  //  enum { Alignment = SuperHeap::Alignment };

  RegionHeap()
    : _sizeRemaining (0),
      _currentArena (nullptr),
      _pastArenas (nullptr),
      _lastChunkSize (ChunkSize)
  {
    static_assert(MultiplierNumerator >= MultiplierDenominator,
		  "Numerator must be at least as large as the denominator.");
    static_assert(MultiplierNumerator * MultiplierDenominator > 0,
		  "Both the numerator and denominator need to be nonzero positive integers.");
  }

  ~RegionHeap()
  {
    clear();
  }

  inline void * __attribute__((always_inline)) malloc (size_t sz) {
    void * ptr;
    // We assume sz is suitably aligned.
    if (unlikely(!_currentArena || (_sizeRemaining < sz))) {
      refill(sz);
      if (!_currentArena) {
	return nullptr;
      }
    }
    //    tprintf::tprintf("bump @ from @\n", sz, _sizeRemaining);
    // Bump the pointer and update the amount of memory remaining.
    _sizeRemaining -= sz;
    ptr = _currentPointer; // Arena->arenaSpace;
    _currentPointer += sz;
    // _currentArena->arenaSpace += sz;
    return ptr;
  }

  /// Free in a zone allocator is a no-op.
  void __attribute__((always_inline)) free (void *) {}
  
  void __attribute__((noinline)) clear()
  {
    Arena * ptr = _pastArenas;
    while (ptr != nullptr) {
      void * oldPtr = (void *) ptr;
      ptr = ptr->nextArena;
      SuperHeap::free (oldPtr);
    }
    if (_currentArena != nullptr) {
      SuperHeap::free ((void *) _currentArena);
    }
    _sizeRemaining = 0;
    _currentArena = nullptr;
    _pastArenas = nullptr;
    _lastChunkSize = ChunkSize;
  }

private:

  size_t getSize(void *);
  
  RegionHeap (const RegionHeap&);
  RegionHeap& operator=(const RegionHeap&);
  
  void __attribute__((noinline)) refill(size_t sz) {
    // Get more space in our arena since there's not enough room in this one.
    // First, add this arena to our past arena list.
    if (_currentArena) {
      _currentArena->nextArena = _pastArenas;
      _pastArenas = _currentArena;
    }
    // Now get more memory.
    size_t allocSize = (int) _lastChunkSize;
    _lastChunkSize *= MultiplierNumerator;
    _lastChunkSize /= MultiplierDenominator;
    if (allocSize < sz) {
      allocSize += sz;
    }
    _currentArena =
      (Arena *) SuperHeap::malloc(allocSize);
    if (_currentArena) {
      assert(_currentArena != nullptr);
      // _currentArena->arenaSpace = (char *) (_currentArena + 1);
      _currentPointer = (char *) (_currentArena + 1);
      _currentArena->nextArena = nullptr;
      _sizeRemaining = allocSize - sizeof(Arena);
    } else {
      _sizeRemaining = 0;
    }
  }
  
  class Arena {
  public:
    Arena() {
      static_assert((sizeof(Arena) % HL::MallocInfo::Alignment == 0),
		    "Alignment must match Arena size.");
    }
    
    //    alignas(8) char * arenaSpace;
    Arena * nextArena;
  };
    
  /// Space left in the current arena.
  size_t _sizeRemaining;
  
  /// The current arena.
  Arena * _currentArena;

  /// The current bump pointer.
  char * _currentPointer;
  
  /// A linked list of past arenas.
  Arena * _pastArenas;

  /// Last size (which increases geometrically).
  float _lastChunkSize;
};

#endif
