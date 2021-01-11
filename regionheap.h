/* -*- C++ -*- */

#pragma once

#ifndef REGIONHEAP_H
#define REGIONHEAP_H

#include <assert.h>

template <class SuperHeap,
	  size_t ChunkSize,
	  unsigned int MultiplierNumerator = 2,
	  unsigned int MultiplierDenominator = 1>
class RegionHeap : public SuperHeap {
public:

  enum { Alignment = SuperHeap::Alignment };

  RegionHeap()
    : _sizeRemaining (0),
      _currentArena (nullptr),
      _pastArenas (nullptr),
      _lastMultiple (1.0)
  {
    static_assert(MultiplierNumerator >= MultiplierDenominator,
		  "Numerator must be at least as large as the denominator.");
    static_assert(MultiplierNumerator * MultiplierDenominator > 0,
		  "Both the numerator and denominator need to be nonzero positive integers.");
  }

  ~RegionHeap()
  {
    Arena * ptr = _pastArenas;
    while (ptr != nullptr) {
      void * oldPtr = (void *) ptr;
      ptr = ptr->nextArena;
      SuperHeap::free (oldPtr);
    }
    if (_currentArena != nullptr)
      SuperHeap::free ((void *) _currentArena);
  }

  inline void * malloc (size_t sz) {
    void * ptr;
    // We assume sz is suitably aligned.
    if (!_currentArena || (_sizeRemaining < sz)) {
      refill();
    }
    // Bump the pointer and update the amount of memory remaining.
    _sizeRemaining -= sz;
    ptr = _currentArena->arenaSpace;
    _currentArena->arenaSpace += sz;
    return ptr;
  }

  /// Free in a zone allocator is a no-op.
  inline void free (void *) {}
  
private:

  size_t getSize(void *);
  
  RegionHeap (const RegionHeap&);
  RegionHeap& operator=(const RegionHeap&);
  
  inline void refill(size_t sz) {
    void * ptr;
    // Get more space in our arena since there's not enough room in this one.
    // First, add this arena to our past arena list.
    if (_currentArena != nullptr) {
      _currentArena->nextArena = _pastArenas;
      _pastArenas = _currentArena;
    }
    // Now get more memory.
    _lastMultiple *= MultiplierNumerator;
    _lastMultiple /= MultiplierDenominator;
    size_t allocSize = ChunkSize * _lastMultiple;
    if (allocSize < sz) {
      allocSize = sz;
    }
    _currentArena =
      (Arena *) SuperHeap::malloc(allocSize + sizeof(Arena));
    assert(_currentArena != nullptr);
    _currentArena->arenaSpace = (char *) (_currentArena + 1);
    _currentArena->nextArena = nullptr;
    _sizeRemaining = allocSize;
  }
  
  class Arena {
  public:
    Arena() {
      static_assert((sizeof(Arena) % HL::MallocInfo::Alignment == 0),
		    "Alignment must match Arena size.");
    }
    
    Arena * nextArena;
    char * arenaSpace;
  };
    
  /// Space left in the current arena.
  size_t _sizeRemaining;
  
  /// The current arena.
  Arena * _currentArena;
  
  /// A linked list of past arenas.
  Arena * _pastArenas;

  /// Running multiplier.
  float _lastMultiplier;
};
