#pragma once

#ifndef FRAGMENTER_HPP
#define FRAGMENTER_HPP

#include <random>
#include <algorithm>
#include <vector>

/***
 * Fragmenter
 *
 * Fragments memory by allocating memory, freeing it randomly,
 * and leaving some fraction allocated.
 *
 * @author Emery Berger <https://www.emeryberger.com>
 *
 ***/

template <size_t MinSize,
  size_t MaxSize,
  size_t NObjects,
  size_t PercentOccupancy>
class Fragmenter {
public:
  Fragmenter()
  {
    static_assert((PercentOccupancy >= 0) && (PercentOccupancy <= 100),
		  "PercentOccupancy must be between 0 and 100.");
    std::random_device rd;
    std::mt19937 g(rd());
    std::vector<void *> allocated;
    std::uniform_int_distribution<> dist(MinSize, MaxSize);

    allocated.reserve(NObjects);
    
    // Allocate a bunch of objects from a range of sizes.
    for (auto i = 0UL; i < NObjects; i++) {
      size_t size = dist(g);
      auto ptr = ::malloc(size);
      allocated[i] = ptr;
    }
    
    // Shuffle them.
    std::shuffle(allocated.begin(), allocated.end(), g);
      
    // Free some fraction of them in shuffled order.
    for (auto i = 0UL; i < ((100 - PercentOccupancy) * NObjects) / 100; i++) {
      auto ptr = allocated[i];
      ::free(ptr);
    }
  }
  
};

#endif
