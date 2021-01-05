#pragma once

#include <stdlib.h>

extern "C" {
  void region_begin(void * buf, size_t sz);
  void region_end();
}

#if defined(__cplusplus)
template <size_t RegionSize>
class cheapen_region {
public:
  cheapen_region()
    : _buf (new char[RegionSize])
    {
      region_begin(_buf, RegionSize);
    }
  ~cheapen_region() {
    region_end();
    delete _buf;
  }
private:
  char * _buf;
};
#endif
