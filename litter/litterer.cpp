#include <atomic>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <vector>
#include <random>
#include <iostream>

// https://json.nlohmann.me
#include "json.hpp"
using json = nlohmann::json;

#include "constants.hpp"

// (NAllocations * LITTER_MULTIPLIER) objects will be allocated by the litterer.
// Then, (1 - LITTER_OCCUPANCY) of them will be freed randomly.
#define LITTER_MULTIPLIER 10
#define LITTER_OCCUPANCY 0.95

static std::chrono::high_resolution_clock::time_point StartTime;

class Initialization {
public:
  Initialization() {
    std::ifstream InputFile(DETECTOR_OUTPUT_FILENAME);
    json Data;
    InputFile >> Data;

    // This can happen if no allocations were recorded.
    if (Data["Bins"].empty()) {
      return;
    }

    long long int NAllocations = 0;
    for (int i = 0; i < Data["Bins"].size(); ++i) {
      NAllocations += Data["Bins"][i].get<int>();
    }
    long long int NAllocationsLitter = NAllocations * LITTER_MULTIPLIER;

    std::vector<std::uniform_int_distribution<size_t>> Distributions;
    Distributions.reserve(Data["Bins"].size());
    Distributions.emplace_back(0, 0);
    for (int i = 1; i < Data["Bins"].size(); ++i) {
      Distributions.emplace_back(((intptr_t) 1) << (i - 1), (((intptr_t) 1) << i) - 1);
    }

    std::cout << "Starting to litter..." << std::endl;

    std::random_device Generator;
    std::uniform_int_distribution<long long int> Distribution(0, NAllocations - 1);
    std::vector<void*> Objects = *(new std::vector<void*>);
    Objects.reserve(NAllocationsLitter);
    for (long long int i = 0; i < NAllocationsLitter; ++i) {
      int DistributionIndex = 0;
      for (int Offset = Distribution(Generator); Offset > 0; Offset -= Data["Bins"][DistributionIndex].get<int>()) {
        ++DistributionIndex;
      }
      void* Pointer = malloc(Distributions[DistributionIndex](Generator));
      Objects.push_back(Pointer);

      if (i % ((long long int) (0.05 * NAllocationsLitter)) == 0) {
        std::cout << "Allocated " << i << " / " << NAllocationsLitter << " (" << (100.0 * i / NAllocationsLitter) <<  "%) objects." << std::endl;
      }
    }

    std::cout << "Shuffling objects..." << std::endl;
    std::shuffle(Objects.begin(), Objects.end(), Generator);
    std::cout << "Freeing objects..." << std::endl;
    long long int NObjectsToBeFreed = (1 - LITTER_OCCUPANCY) * NAllocationsLitter;
    for (long long int i = 0; i < NObjectsToBeFreed; ++i) {
      free(Objects[i]);
    }

    std::cout << "Finished littering." << std::endl;

    StartTime = std::chrono::high_resolution_clock::now();
  }

  ~Initialization() {
    std::chrono::high_resolution_clock::time_point EndTime = std::chrono::high_resolution_clock::now();
    std::ofstream OutputFile(LITTERER_OUTPUT_FILENAME, std::ios_base::app);
    OutputFile << (EndTime - StartTime).count() << std::endl;
  }
};

static Initialization _;