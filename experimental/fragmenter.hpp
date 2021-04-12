#pragma once

#ifndef FRAGMENTER_HPP
#define FRAGMENTER_HPP

#include <random>
#include <algorithm>
#include <vector>
#include <ratio>
#include <utility>

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
	  size_t OccupancyNumerator,
	  size_t OccupancyDenominator,
	  size_t Seed = 0>
class Fragmenter {
public:
  Fragmenter()
  {
    std::random_device rd;
    std::mt19937 * g;
    // If there is a non-zero seed set, use it; otherwise, use the
    // random device.
    size_t seed;
    if (Seed) {
      seed = Seed;
    } else {
      seed = rd();
    }
    g = new std::mt19937(seed);
    //    printf("seed = %lu\n", seed);
    std::vector<void *> allocated;
    std::uniform_int_distribution<> dist(MinSize, MaxSize);

    allocated.reserve(NObjects);
    
    // Allocate a bunch of objects from a range of sizes.
    for (auto i = 0UL; i < NObjects; i++) {
      size_t size = dist(*g);
      auto ptr = ::malloc(size);
      allocated[i] = ptr;
    }
    
    // Shuffle them.
    std::shuffle(allocated.begin(), allocated.end(), *g);
      
    // Free some fraction of them in shuffled order.
    for (auto i = 0UL; i < ((OccupancyDenominator - OccupancyNumerator) * NObjects) / OccupancyDenominator; i++) {
      auto ptr = allocated[i];
      ::free(ptr);
    }
  }
  
};

#endif
