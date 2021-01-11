#pragma once

#include <stdlib.h>
#include <malloc.h>

extern "C" {
void region_begin(void *buf, size_t sz, bool allAligned = false,
                  bool allNonZero = false, bool sizeTaken = true);
void region_end();
}

#if defined(__cplusplus)

namespace cheap {

enum flags {
  ALIGNED = 0b0000'0001,
  NONZERO = 0b0000'0010,
  SIZE_TAKEN = 0b0000'0100,
  SINGLE_THREADED = 0b0000'1000,
  REGION = 0b0001'0000
};

template <size_t RegionSize> class cheap {
public:
  inline cheap(int f) {
    if (RegionSize <= 1048576) {
      _buf = reinterpret_cast<char *>(alloca(RegionSize));
    } else {
      _buf = new char[RegionSize];
    }
    region_begin(_buf, RegionSize, f & flags::ALIGNED, f & flags::NONZERO,
                 f & flags::SIZE_TAKEN);
  }
  inline ~cheap() {
    region_end();
    if (RegionSize > 1048576) {
      delete[] _buf;
    }
  }

private:
  char *_buf;
};

} // namespace cheap

#endif
