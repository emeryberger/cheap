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
  size_t _seed {0};
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
    if (Seed) {
      _seed = Seed;
    } else {
      _seed = rd();
    }
    gen = new std::mt19937(_seed);
    std::vector<void *> allocated = *(new std::vector<void*>(NObjects));
    std::vector<void *> allocated_copy = *(new std::vector<void*>(NObjects));
    std::uniform_int_distribution<> dist(MinSize, MaxSize);

    intptr_t minAddress = -1;
    intptr_t maxAddress = -1;

    // Allocate a bunch of objects from a range of sizes.
    for (auto i = 0UL; i < NObjects; i++) {
      size_t size = dist(*gen);
      auto ptr = ::malloc(size);
      allocated[i] = ptr;
      allocated_copy[i] = ptr;

      if ((intptr_t) ptr < minAddress || minAddress == -1) {
        minAddress = (intptr_t) ptr;
      }

      if ((intptr_t) ptr > maxAddress || maxAddress == -1) {
        maxAddress = (intptr_t) ptr;
      }
    }

    std::cout << "Allocated " << NObjects << " objects..." << std::endl;
    std::cout << "Min: " << minAddress << ", Max: " << maxAddress << ", Spread: " << (maxAddress - minAddress) << std::endl;

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

  size_t getSeed() const {
    return _seed;
  }
};

#endif
