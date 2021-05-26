#pragma once

#ifndef SHUFFLER_HPP
#define SHUFFLER_HPP

#include <cassert>
#include <random>
#include <algorithm>
#include <vector>
#include <ratio>
#include <utility>

/***
 * Shuffler
 *
 * Shuffles the heap by allocating memory, freeing it randomly,
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
	  bool Shuffle = true,
	  size_t Seed = 0>
class Shuffler {
private:
  std::mt19937 * gen { nullptr };
public:
  Shuffler()
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
    
    if (Shuffle) {
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
    for (auto i = 0UL; i < ((OccupancyDenominator - OccupancyNumerator) * NObjects) / OccupancyDenominator; i++) {
      auto ptr = allocated[i];
      // std::cout << "freeing " << ptr << std::endl;
      ::free(ptr);
    }
  }

  ~Shuffler() {
    delete gen;
  }
};

#endif
