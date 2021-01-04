#include <stdlib.h>

extern "C" {
  void region_begin(void * buf, size_t sz);
  void region_end();
}

