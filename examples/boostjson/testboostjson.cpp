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


void parseMe(const char * s, size_t sz)
{
  //  boost::json::stream_parser p;
  boost::json::parser p;
  boost::json::error_code ec;
#if 1
  p.write(s, sz, ec);
#endif
#if 0
  if (!ec) {
    p.finish(ec);
    volatile auto& pv = p;
  }
  if (!ec) {
    auto jv = p.release();
    volatile auto& v = jv;
  }
  volatile auto& pv = p;
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
  auto sz = str.size();
  for (auto i = 0; i < 1000; i++)
  {
#if CHEAPEN
    cheap::cheap reg(cheap::NONZERO | cheap::SINGLE_THREADED);
#endif
    parseMe(data, sz);
  }
  return 0;
}
