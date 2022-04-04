#include <atomic>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "constants.hpp"

static std::atomic_bool Ready{false};
static thread_local int Busy{0};

#ifndef SIZE_CLASSES
// Default: Powers of 2 up to 2 ** 30
#define SIZE_CLASSES                                                                                                   \
    {                                                                                                                  \
        2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288,       \
            1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912,         \
            1073741824                                                                                                 \
    }
#endif

static const std::vector<size_t> SizeClasses = SIZE_CLASSES;
static std::vector<std::atomic_int> Bins(SizeClasses.size() + 1);

static std::atomic_int64_t NAllocations{0};
static std::atomic<double> Average{0};

// Increments by 1 on malloc/calloc, and decrements by 1 on free.
static std::atomic_int64_t LiveAllocations{0};
static std::atomic_int64_t MaxLiveAllocations{0};

class Initialization {
  public:
    Initialization() { Ready = true; }

    ~Initialization() {
        Ready = false;

        std::ofstream OutputFile(DETECTOR_OUTPUT_FILENAME);

        OutputFile << "{" << std::endl;

        if (SizeClasses.size() == 0) {
            OutputFile << "\t\"SizeClasses\": []," << std::endl;
        } else {
            OutputFile << "\t\"SizeClasses\": [ " << SizeClasses[0];
            for (int i = 1; i < SizeClasses.size(); ++i) {
                OutputFile << ", " << SizeClasses[i];
            }
            OutputFile << " ]," << std::endl;
        }

        OutputFile << "\t\"Bins\": [ " << Bins[0];
        for (int i = 1; i < Bins.size(); ++i) {
            OutputFile << ", " << Bins[i];
        }
        OutputFile << "]," << std::endl;

        OutputFile << "\t\"NAllocations\": " << NAllocations << ", \"Average\": " << Average
                   << ", \"MaxLiveAllocations\": " << MaxLiveAllocations << std::endl;
        OutputFile << "}" << std::endl;
    }
};

static Initialization _;

extern "C" ATTRIBUTE_EXPORT void* malloc(size_t Size) noexcept {
    static decltype(::malloc)* Malloc = (decltype(::malloc)*) dlsym(RTLD_NEXT, "malloc");

    void* Pointer = (*Malloc)(Size);

    if (!Busy && Ready) {
        ++Busy;

        int index = 0;
        while (Size > SizeClasses[index] && index < SizeClasses.size()) {
            ++index;
        }
        Bins[index]++;

        Average = Average + (Size - Average) / (NAllocations + 1);
        NAllocations++;

        long int LiveAllocationsSnapshot = LiveAllocations.fetch_add(1) + 1;
        long int MaxLiveAllocationsSnapshot = MaxLiveAllocations;
        while (LiveAllocationsSnapshot > MaxLiveAllocationsSnapshot) {
            MaxLiveAllocations.compare_exchange_weak(MaxLiveAllocationsSnapshot, LiveAllocationsSnapshot);
            MaxLiveAllocationsSnapshot = MaxLiveAllocations;
        }

        --Busy;
    }

    return Pointer;
}

extern "C" ATTRIBUTE_EXPORT void* calloc(size_t N, size_t Size) noexcept {
    static decltype(::calloc)* Calloc = (decltype(::calloc)*) dlsym(RTLD_NEXT, "calloc");

    void* Pointer = (*Calloc)(N, Size);

    size_t TotalSize = N * Size;
    if (!Busy && Ready) {
        ++Busy;

        int index = 0;
        while (Size > SizeClasses[index] && index < SizeClasses.size()) {
            ++index;
        }
        Bins[index]++;

        Average = Average + (TotalSize - Average) / (NAllocations + 1);
        NAllocations++;

        long int LiveAllocationsSnapshot = LiveAllocations.fetch_add(1) + 1;
        long int MaxLiveAllocationsSnapshot = MaxLiveAllocations;
        while (LiveAllocationsSnapshot > MaxLiveAllocationsSnapshot) {
            MaxLiveAllocations.compare_exchange_weak(MaxLiveAllocationsSnapshot, LiveAllocationsSnapshot);
            MaxLiveAllocationsSnapshot = MaxLiveAllocations;
        }

        --Busy;
    }

    return Pointer;
}

extern "C" ATTRIBUTE_EXPORT void free(void* Pointer) noexcept {
    static decltype(::free)* Free = (decltype(::free)*) dlsym(RTLD_NEXT, "free");

    if (!Busy && Ready) {
        ++Busy;
        LiveAllocations--;
        --Busy;
    }

    (*Free)(Pointer);
}