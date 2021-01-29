#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

#define OPTIMIZED_HEAP 0 // NB: NOT working

#if CHEAPEN
#include "cheap.h"
#endif

// #include <boost/json/src.hpp>
#include "src.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#include <memory_resource>

void parseMe(volatile const char * s, size_t sz)
{
#if OPTIMIZED_HEAP
  std::pmr::monotonic_buffer_resource mr;
  std::pmr::polymorphic_allocator<> pa{ &mr };
  auto &p = *pa.new_object<boost::json::stream_parser>();
  p.reset(pa);
#endif
  boost::json::error_code ec;
  
#if 0
  boost::json::parser ps;
  ps.write(s, sz, ec);
#else
  boost::json::stream_parser ps;
  ps.write((const char *) s, sz, ec);
  if (!ec) {
    ps.finish(ec);
  }
#if !OPTIMIZED_HEAP
  if (!ec) {
#if !CHEAPEN
    auto jv = ps.release();
#endif
  }
#endif
#endif
}

int main()
{
  //   std::ifstream i("gsoc-2018.json");
  std::ifstream i("citm_catalog.json");
  std::ostringstream sstr;
  sstr << i.rdbuf();
  //  mallopt(M_MMAP_THRESHOLD, 10487560 + 32);
  auto str = sstr.str();
  auto data = str.data();
  volatile auto sz = str.size();
  char * buf = new char[3 * 1048576];
  for (auto i = 0; i < 1000; i++)
  {
#if CHEAPEN
    cheap::cheap<cheap::DISABLE_FREE | cheap::NONZERO | cheap::SINGLE_THREADED | cheap::FIXED_BUFFER> reg(8, buf, 3 * 1048576);
    // cheap::cheap<cheap::NONZERO | cheap::SINGLE_THREADED | cheap::DISABLE_FREE> reg;
#endif
    parseMe(data, sz);
  }
  return 0;
}
