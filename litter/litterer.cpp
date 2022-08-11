#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include <dlfcn.h>
#include <unistd.h>

#ifdef OUTPUT_PERF_DATA
#include "PFMWrapper.hpp"
#endif

// https://json.nlohmann.me
#include "json.hpp"
using json = nlohmann::json;

#include "constants.hpp"

class Initialization {
  public:
    Initialization() {
        #ifdef OUTPUT_PERF_DATA
        PFMWrapper::initialize();
        PFMWrapper::addEvent("instructions");
        PFMWrapper::addEvent("cycles");
        PFMWrapper::addEvent("L1-dcache-loads");
        PFMWrapper::addEvent("L1-dcache-load-misses");
        PFMWrapper::addEvent("LLC-loads");
        PFMWrapper::addEvent("LLC-load-misses");
        PFMWrapper::addEvent("dTLB-loads");
        PFMWrapper::addEvent("dTLB-load-misses");
        #endif

        auto Seed = std::random_device{}();
        if (const char* env = std::getenv("LITTER_SEED")) {
            Seed = atoi(env);
        }
        std::mt19937_64 Generator(Seed);

        auto Occupancy = 0.95;
        if (const char* env = std::getenv("LITTER_OCCUPANCY")) {
            Occupancy = atof(env);
            assert(Occupancy >= 0 && Occupancy <= 1);
        }

        bool NoShuffle = false;
        if (const char* env = std::getenv("LITTER_NO_SHUFFLE")) {
            NoShuffle = atoi(env) != 0;
        }

        unsigned Sleep = 0;
        if (const char* env = std::getenv("LITTER_SLEEP")) {
            Sleep = atoi(env);
        }

        auto Multiplier = 20;
        if (const char* env = std::getenv("LITTER_MULTIPLIER")) {
            Multiplier = atoi(env);
        }

        Dl_info MallocInformation;
        assert(dladdr((void*) &malloc, &MallocInformation) != 0);
        std::cerr << "==================================== Litterer ====================================" << std::endl;
        std::cerr << "malloc     : " << MallocInformation.dli_fname << std::endl;
        std::cerr << "seed       : " << Seed << std::endl;
        std::cerr << "occupancy  : " << Occupancy << std::endl;
        std::cerr << "shuffle    : " << (NoShuffle ? "no" : "yes") << std::endl;
        std::cerr << "sleep      : " << (Sleep ? std::to_string(Sleep) : "no") << std::endl;
        std::cerr << "multiplier : " << Multiplier << std::endl;
        std::cerr << "timestamp  : " << __DATE__ << " " << __TIME__ << std::endl;
        std::cerr << "==================================================================================" << std::endl;

        std::ifstream InputFile(DETECTOR_OUTPUT_FILENAME);
        json Data;
        InputFile >> Data;

        long long int NAllocations = Data["NAllocations"].get<long long int>();
        long long int MaxLiveAllocations = Data["MaxLiveAllocations"].get<long long int>();
        long long int NAllocationsLitter = MaxLiveAllocations * Multiplier;

        // This can happen if no allocations were recorded.
        if (!Data["Bins"].empty()) {
            if (Data["Bins"][Data["SizeClasses"].size()].get<int>() != 0) {
                std::cerr << "WARNING: Allocations of size greater than the maximum size class were recorded." << std::endl;
                std::cerr << "WARNING: There will be no littering for these allocations." << std::endl;
                std::cerr << "WARNING: This represents "
                        << ((double) Data["Bins"][Data["SizeClasses"].size()] / (double) NAllocations) * 100
                        << "% of all allocations recorded." << std::endl;
            }

            std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();

            std::uniform_int_distribution<long long int> Distribution(
                0, NAllocations - Data["Bins"][Data["SizeClasses"].size()].get<int>() - 1);
            std::vector<void*> Objects = *(new std::vector<void*>);
            Objects.reserve(NAllocationsLitter);

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
            }

            long long int NObjectsToBeFreed = (1 - Occupancy) * NAllocationsLitter;

            if (NoShuffle) {
                std::sort(Objects.begin(), Objects.end(), std::greater<void*>());
            } else {
                for (int i = 0; i < NObjectsToBeFreed; ++i) {
                    std::uniform_int_distribution<int> IndexDistribution(i, Objects.size() - 1);
                    int Index = IndexDistribution(Generator);
                    void* Temporary = Objects[i];
                    Objects[i] = Objects[Index];
                    Objects[Index] = Temporary;
                }
            }

            for (long long int i = 0; i < NObjectsToBeFreed; ++i) {
                free(Objects[i]);
            }

            std::chrono::high_resolution_clock::time_point EndTime = std::chrono::high_resolution_clock::now();
            std::chrono::seconds Elapsed = std::chrono::duration_cast<std::chrono::seconds>((EndTime - StartTime));
            std::cerr << "Finished littering. Time taken: " << Elapsed.count() << " seconds." << std::endl;
        }

        if (Sleep) {
            std::cerr << "Sleeping " << Sleep << " seconds before starting the program (PID: " << getpid()
                    << ")..." << std::endl;
            sleep(Sleep);
            std::cerr << "Starting program now!" << std::endl;
        }

        std::cerr << "==================================================================================" << std::endl;

        #ifdef OUTPUT_PERF_DATA
	    PFMWrapper::start();
        #endif
    }

    #ifdef OUTPUT_PERF_DATA
    ~Initialization() {
        PFMWrapper::stop();
        PFMWrapper::print();
        PFMWrapper::cleanup();
    }
    #endif
};

static Initialization _;
