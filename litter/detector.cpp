#include <atomic>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "constants.hpp"

static std::atomic_bool Ready{false};
static thread_local int Busy{0};

#ifndef SIZE_CLASSES
// See http://jemalloc.net/jemalloc.3.html, up to 64MiB.
#define JEMALLOC_SIZE_CLASSES                                                                                                   \
    {                                                                                                                  \
        8, 16, 32, 48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 640, 768, 896, 1024, 1280, 1536,  \
            1792, 2048, 2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384, 20480, 24576,      \
            28672, 32768, 40960, 49152, 57344, 65536, 81920, 98304, 114688, 131072, 163840, 196608, 229376, 262144,    \
            327680, 393216, 458752, 524288, 655360, 786432, 917504, 1048576, 1310720, 1572864, 1835008, 2097152,       \
            2621440, 3145728, 3670016, 4194304, 5242880, 6291456, 7340032, 8388608, 10485760, 12582912, 14680064,      \
            16777216, 20971520, 25165824, 29360128, 33554432, 41943040, 50331648, 58720256, 67108864                   \
    }

// Default: Powers of 2 up to 2 ** 30
// #define SIZE_CLASSES \
//     { \
//         2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, \
//             1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912, \
//             1073741824 \
//     }
#define SIZE_CLASSES JEMALLOC_SIZE_CLASSES
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