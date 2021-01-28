#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

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
  boost::json::error_code ec;
#if 0
  boost::json::parser p;
  p.write(s, sz, ec);
#else
  boost::json::stream_parser p;
  p.write((const char *) s, sz, ec);
  if (!ec) {
    p.finish(ec);
  }
  if (!ec) {
#if !CHEAPEN
    auto jv = p.release();
#endif
  }
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
