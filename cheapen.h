#pragma once

#include <stdlib.h>

extern "C" {
  void region_begin(void *buf, size_t sz, bool allAligned = false, bool allNonZero = false, bool sizeTaken = true);
  void region_end();
}

#if defined(__cplusplus)
template <size_t RegionSize,
	  bool AllAligned = false,
	  bool AllNonZero = false,
	  bool SizeTaken = true>
class cheapen_region {
public:
  cheapen_region() : _buf(new char[RegionSize]) {
    region_begin(_buf, RegionSize, AllAligned, AllNonZero, SizeTaken);
  }
  ~cheapen_region() {
    region_end();
    delete[] _buf;
  }

private:
  char *_buf;
};
#endif
