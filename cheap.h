#pragma once

#include <stdlib.h>
#include <malloc.h>

extern "C" {
void region_begin(bool allAligned = false,
                  bool allNonZero = false, bool sizeTaken = true,
		  bool sameSize = false, size_t oneSize = 8);
void region_end();
}

#if defined(__cplusplus)

namespace cheap {

enum flags {
  ALIGNED = 0b0000'0001,
  NONZERO = 0b0000'0010,
  SIZE_TAKEN = 0b0000'0100,
  SINGLE_THREADED = 0b0000'1000,
  DISABLE_FREE = 0b0001'0000,
  SAME_SIZE = 0b0010'0000,
};

 
class cheap {
public:
  inline cheap(int f, size_t sz = 8) {
    static_assert(flags::ALIGNED ^ flags::NONZERO ^ flags::SIZE_TAKEN ^ flags::SINGLE_THREADED ^ flags::DISABLE_FREE ^ flags::SAME_SIZE == (1 << 7) - 1,
		  "Flags must be one bit and mutually exclusive.");
    region_begin(f & flags::ALIGNED, f & flags::NONZERO,
                 f & flags::SIZE_TAKEN, f & flags::SAME_SIZE, sz);
  }
  inline ~cheap() {
    region_end();
  }
};

} // namespace cheap

#endif
