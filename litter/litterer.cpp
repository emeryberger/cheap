#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <dlfcn.h>

// https://json.nlohmann.me
#include "json.hpp"
using json = nlohmann::json;

#include "constants.hpp"

// (MaxLiveAllocations * LITTER_MULTIPLIER) objects will be allocated by the litterer.
// Then, (1 - LITTER_OCCUPANCY) of them will be freed randomly.

#ifndef LITTER_MULTIPLIER
#define LITTER_MULTIPLIER 200
#endif

#ifndef LITTER_OCCUPANCY
#define LITTER_OCCUPANCY 0.95
#endif

class Initialization {
  public:
    Initialization() {
        Dl_info MallocInformation;
        assert(dladdr((void*) &malloc, &MallocInformation) != 0);
        std::cerr << "Using malloc from: " << MallocInformation.dli_fname << std::endl;

        std::ifstream InputFile(DETECTOR_OUTPUT_FILENAME);
        json Data;
        InputFile >> Data;

        long long int NAllocations = Data["NAllocations"].get<long long int>();
        long long int MaxLiveAllocations = Data["MaxLiveAllocations"].get<long long int>();
        long long int NAllocationsLitter = MaxLiveAllocations * LITTER_MULTIPLIER;

        // This can happen if no allocations were recorded.
        if (Data["Bins"].empty()) {
            return;
        }

        if (Data["Bins"][Data["SizeClasses"].size()].get<int>() != 0) {
            std::cerr << "WARNING: Allocations of size greater than the maximum size class were recorded." << std::endl;
            std::cerr << "WARNING: There will be no littering for these allocations." << std::endl;
            std::cerr << "WARNING: This represents "
                      << ((double) Data["Bins"][Data["SizeClasses"].size()] / (double) NAllocations) * 100
                      << "% of all allocations recorded." << std::endl;
        }

        std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();

        std::random_device Generator;
        std::uniform_int_distribution<long long int> Distribution(
            0, NAllocations - Data["Bins"][Data["SizeClasses"].size()].get<int>() - 1);
        std::vector<void*> Objects = *(new std::vector<void*>);
        Objects.reserve(NAllocationsLitter);

        int Percentage = -1;
        for (long long int i = 0; i < NAllocationsLitter; ++i) {
            size_t MinAllocationSize = 0;
            size_t SizeClassIndex = 0;
            long long int Offset = Distribution(Generator) - (long long int) Data["Bins"][0].get<int>();

            while (Offset >= 0LL) {
                MinAllocationSize = Data["SizeClasses"][SizeClassIndex].get<size_t>() + 1;
                ++SizeClassIndex;
                Offset -= (long long int) Data["Bins"][SizeClassIndex].get<int>();
            }
            size_t MaxAllocationSize = Data["SizeClasses"][SizeClassIndex].get<size_t>();
            std::uniform_int_distribution<size_t> AllocationSizeDistribution(MinAllocationSize, MaxAllocationSize);
            size_t AllocationSize = AllocationSizeDistribution(Generator);

            void* Pointer = malloc(AllocationSize);
            Objects.push_back(Pointer);

            int NewPercentage = 100.0 * (i + 1) / NAllocationsLitter;
            if (NewPercentage > Percentage) {
                Percentage = NewPercentage;
                std::cerr << "\rAllocated " << i + 1 << " / " << NAllocationsLitter << " (" << Percentage
                          << "%) objects.";
            }
        }

        std::cerr << std::endl;

        long long int NObjectsToBeFreed = (1 - LITTER_OCCUPANCY) * NAllocationsLitter;
        std::cerr << "Shuffling objects..." << std::endl;
        for (int i = 0; i < NObjectsToBeFreed; ++i) {
            std::uniform_int_distribution<int> IndexDistribution(i, Objects.size() - 1);
            int Index = IndexDistribution(Generator);
            void* Temporary = Objects[i];
            Objects[i] = Objects[Index];
            Objects[Index] = Temporary;
        }

        for (long long int i = 0; i < NObjectsToBeFreed; ++i) {
            free(Objects[i]);
        }

        std::chrono::high_resolution_clock::time_point EndTime = std::chrono::high_resolution_clock::now();
        std::chrono::seconds Elapsed = std::chrono::duration_cast<std::chrono::seconds>((EndTime - StartTime));
        std::cerr << "Finished littering. Time taken: " << Elapsed.count() << " seconds." << std::endl;
    }
};

static Initialization _;