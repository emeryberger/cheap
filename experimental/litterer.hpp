#pragma once

#ifndef LITTERER_HPP
#define LITTERER_HPP

#include <cassert>
#include <random>
#include <algorithm>
#include <vector>
#include <ratio>
#include <utility>

/***
 * Litterer
 *
 * Shuffles the heap by allocating memory, freeing it randomly,
 * and leaving some fraction allocated.
 *
 * @author Emery Berger <https://www.emeryberger.com>
 *
 ***/

class Litterer {
private:
  std::mt19937 * gen { nullptr };
public:
  Litterer(size_t MinSize,
	   size_t MaxSize,
	   size_t NObjects,
	   size_t OccupancyNumerator,
	   size_t OccupancyDenominator,
	   bool shuffle = true,
	   size_t Seed = 0)
  {
    std::random_device rd;
    // If there is a non-zero seed set, use it; otherwise, use the
    // random device.
    size_t seed;
    if (Seed) {
      seed = Seed;
    } else {
      seed = rd();
    }
    gen = new std::mt19937(seed);
    std::vector<void *> allocated (NObjects);
    std::vector<void *> allocated_copy (NObjects);
    std::uniform_int_distribution<> dist(MinSize, MaxSize);

    // Allocate a bunch of objects from a range of sizes.
    for (auto i = 0UL; i < NObjects; i++) {
      size_t size = dist(*gen);
      auto ptr = ::malloc(size);
      allocated[i] = ptr;
      allocated_copy[i] = ptr;
    }

    assert(allocated.size() == NObjects);
    
    if (shuffle) {
      // Shuffle them.
      std::shuffle(allocated.begin(), allocated.end(), *gen);
      // Prevent optimization.
      volatile auto v = allocated.end();
    } else {
      // Shuffle the copy.
      // Shuffle them.
      std::shuffle(allocated_copy.begin(), allocated_copy.end(), *gen);
      // Prevent optimization.
      volatile auto v = allocated_copy.end();
    }
    
    // Free some fraction of them (potentially in shuffled order).
    auto objsToBeFreed = ((OccupancyDenominator - OccupancyNumerator) * NObjects) / OccupancyDenominator;
    for (auto i = 0UL; i < objsToBeFreed; i++) {
      auto ptr = allocated[i];
      ::free(ptr);
    }
  }

  ~Litterer() {
    delete gen;
  }
};

#endif
