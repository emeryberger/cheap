#pragma once

#ifndef LITTERER_HPP
#define LITTERER_HPP

#include <cassert>
#include <random>
#include <algorithm>
#include <iostream>
#include <vector>
#include <ratio>
#include <utility>

#include <iostream>

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
	   size_t Seed = 0)
  {
    std::random_device rd;
    std::cout << "Littering." << std::endl;
    // If there is a non-zero seed set, use it; otherwise, use the
    // random device.
    if (Seed) {
      _seed = Seed;
    } else {
      _seed = rd();
    }
    gen = new std::mt19937(_seed);
    std::vector<void *> allocated = *(new std::vector<void*>(NObjects));
    std::uniform_int_distribution<> dist(MinSize, MaxSize);

    intptr_t minAddress = -1;
    intptr_t maxAddress = -1;

    // Allocate a bunch of objects from a range of sizes.
    for (auto i = 0UL; i < NObjects; i++) {
      size_t size = dist(*gen);
      auto ptr = ::malloc(size);
      allocated[i] = ptr;

      if ((intptr_t) ptr < minAddress || minAddress == -1) {
        minAddress = (intptr_t) ptr;
      }

      if ((intptr_t) ptr > maxAddress || maxAddress == -1) {
        maxAddress = (intptr_t) ptr;
      }
    }

#ifndef NODEBUG
    std::cout << "Allocated " << NObjects << " objects..." << std::endl;
    std::cout << "Min: " << minAddress << ", Max: " << maxAddress << ", Spread: " << (maxAddress - minAddress) << std::endl;
#endif

    assert(allocated.size() == NObjects);
    
    // Shuffle them.
    std::shuffle(allocated.begin(), allocated.end(), *gen);
    // Prevent optimization.
    volatile auto v = allocated.end();
    
    // Free some fraction of them (potentially in shuffled order).
    auto objsToBeFreed = ((OccupancyDenominator - OccupancyNumerator) * NObjects) / OccupancyDenominator;
    for (auto i = 0UL; i < objsToBeFreed; i++) {
      auto ptr = allocated[i];
      ::free(ptr);
    }
#ifndef NODEBUG
    std::cout << "Littering complete." << std::endl;
#endif
  }

  ~Litterer() {
    delete gen;
  }

  size_t getSeed() const {
    return _seed;
  }
};

#endif
