#include <atomic>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "constants.hpp"

static std::atomic_bool Ready{false};
static thread_local int Busy{0};

static std::vector<std::atomic_int> Bins(sizeof(intptr_t) * 8);
static std::atomic_int64_t NAllocations{0};
static std::atomic<double> Average{0};

class Initialization {
public:
  Initialization() { Ready = true; }
  ~Initialization() {
    Ready = false;

    std::ofstream OutputFile(DETECTOR_OUTPUT_FILENAME);

    int MaxI;
    for (int i = 0; i < Bins.size(); ++i) {
      if (Bins[i] > 0) {
        MaxI = i;
      }
    }

    OutputFile << "{ \"Bins\": [ " << Bins[0];
    for (int i = 1; i <= MaxI; ++i) {
      OutputFile << ", " << Bins[i];
    }
    OutputFile << "], \"NAllocations\": " << NAllocations << ", \"Average\": " << Average << " }" << std::endl;
  }
};

static Initialization _;

extern "C" ATTRIBUTE_EXPORT void* malloc(size_t Size) noexcept {
  static decltype(::malloc)* Malloc = (decltype(::malloc)*) dlsym(RTLD_NEXT, "malloc");

  void* Pointer = (*Malloc)(Size);

  if (!Busy && Ready) {
    ++Busy;

    for (int i = 0; i < Bins.size(); ++i) {
      if (Size >> i == 0) {
        Bins[i]++;
        break;
      }
    }

    Average = Average + (Size - Average) / (NAllocations + 1);
    NAllocations++;

    --Busy;
  }

  return Pointer;
}