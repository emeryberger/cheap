#pragma once

#ifndef PAGE_LITTERER_HPP
#define PAGE_LITTERER_HPP

#include <algorithm> // std::sort
#include <cassert>
#ifndef __APPLE__
#include <malloc.h> // malloc_usable_size
#else
#include <malloc/malloc.h>
#define malloc_usable_size(x) malloc_size(x)
#endif
#include <random>
#include <set>

#ifndef __APPLE__
long abs(long l) { return (l < 0L) ? -l : l; }
#endif

/***
 * The intuition behind this first version is to keep allocating until we reach PageSize bytes, forming a group of at
 * least that size. Repeat to obtain NPages groups of at least PageSize bytes. Then, free a single object from each.
 ***/
class PageLittererV1 {
public:
  PageLittererV1(int MinSize, int MaxSize, int NPages, size_t Seed, long PageSize = sysconf(_SC_PAGESIZE)) {
    std::vector<std::vector<void*>> allocated = *(new std::vector<std::vector<void*>>(NPages));
    std::default_random_engine gen(Seed);
    std::uniform_int_distribution<int> dist(MinSize, MaxSize);

    int NAllocations = 0;
    int pagesFilled = 0;
    long currentPageFill = 0;

    intptr_t minAddress = -1;
    intptr_t maxAddress = -1;

    int AverageObjectSize = (MinSize + MaxSize) / 2;
    for (int i = 0; i < NPages; ++i) {
      allocated[i].reserve((PageSize / AverageObjectSize) * 1.05);
    }

    while (pagesFilled != NPages) {
      size_t size = dist(gen);
      auto ptr = ::malloc(size);
      allocated[pagesFilled].push_back(ptr);
      ++NAllocations;

      if (minAddress == -1 || ((intptr_t) ptr) < minAddress) {
        minAddress = (intptr_t) ptr;
      }

      if (maxAddress == -1 || ((intptr_t) ptr) > maxAddress) {
        maxAddress = (intptr_t) ptr;
      }

      currentPageFill += ::malloc_usable_size(ptr);
      if (currentPageFill >= PageSize) {
        ++pagesFilled;
        currentPageFill = 0;
      }
    }

    std::vector<void*> toBeFreed = *(new std::vector<void*>);
    toBeFreed.reserve(NPages);
    for (int i = 0; i < NPages; ++i) {
      std::uniform_int_distribution<int> dist(0, allocated[i].size() - 1);
      int index = dist(gen);
      toBeFreed.push_back(allocated[i][index]);
    }

    std::shuffle(toBeFreed.begin(), toBeFreed.end(), gen);
    for (auto ptr : toBeFreed) {
      free(ptr);
    }

    std::cout << "Allocated " << NAllocations << " objects..." << std::endl;
    std::cout << "Min: " << minAddress << ", Max: " << maxAddress << ", Spread: " << (maxAddress - minAddress) << std::endl;
  }
};

/***
 * This second method allocates objects, then sorts the addresses, and proceeds to free an object at least every
 * PageSize bytes. Some corner cases need to be handled, mainly because it is hard to know how many objects we will
 * actually need to allocate to be able to partition enough pages.
 ***/
class PageLittererV2 {
public:
  PageLittererV2(int MinSize, int MaxSize, int NPages, size_t Seed, long PageSize = sysconf(_SC_PAGESIZE)) {
    std::vector<void*> allocated = *(new std::vector<void*>);
    std::default_random_engine gen(Seed);
    std::uniform_int_distribution<int> dist(MinSize, MaxSize);

    int AverageObjectSize = (MinSize + MaxSize) / 2;
    // We add 5% to our initial guess to make it quasi-impossible to have a second set of allocations.
    int NAllocations = guess(AverageObjectSize, 0, 0, NPages, PageSize) * 1.05;
    int PagesFilled = 0;

    while (PagesFilled < NPages) {
      allocated.reserve(NAllocations);

      for (int i = 0; i < NAllocations; ++i) {
        size_t size = dist(gen);
        auto ptr = ::malloc(size);
        allocated.push_back(ptr);
      }

      // Recount how many pages our current allocations are filling.
      std::sort(allocated.begin(), allocated.end());
      PagesFilled = 0;
      intptr_t previous = (intptr_t) allocated[0];
      for (int i = 1; i < allocated.size(); ++i) {
        if (abs((intptr_t) allocated[i] - previous) >= PageSize) {
          previous = (intptr_t) allocated[i];
          ++PagesFilled;
        }
      }

      // If we fell short, allocate more.
      if (PagesFilled < NPages) {
        NAllocations = guess(AverageObjectSize, NAllocations, PagesFilled, NPages, PageSize);
      }
    }

    // allocated is already sorted.
    
    std::cout << "Allocated " << NAllocations << " objects..." << std::endl;
    intptr_t minAddress = (intptr_t) allocated[0];
    intptr_t maxAddress = (intptr_t) allocated[allocated.size() - 1];
    std::cout << "Min: " << minAddress << ", Max: " << maxAddress << ", Spread: " << (maxAddress - minAddress) << std::endl;

    std::vector<void*> toBeFreed = *(new std::vector<void*>);
    toBeFreed.reserve(PagesFilled);

    intptr_t previous = (intptr_t) allocated[0];
    for (int i = 1; i < allocated.size(); ++i) {
      if (abs((intptr_t) allocated[i] - previous) >= PageSize) {
        toBeFreed.push_back((void*) previous);
        previous = (intptr_t) allocated[i];
      }
    }
    
    std::shuffle(toBeFreed.begin(), toBeFreed.end(), gen);
    for (auto ptr : toBeFreed) {
      free(ptr);
    }

    assert(PagesFilled >= NPages);
  }

private:
  // Estimate total allocations that would be needed to reach the goal.
  int guess(int AverageObjectSize, int AlreadyAllocated, int PagesFilled, int TargetPages, long PageSize) {
    return (AlreadyAllocated && PagesFilled) ? ((long long int) AlreadyAllocated) * TargetPages / PagesFilled
                                             : ((long long int) TargetPages) * PageSize / AverageObjectSize;
  }
};

#endif
