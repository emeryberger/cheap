#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

#if CHEAPEN
#include "cheap.h"
#endif

#include "json.hpp"

void parseMe(std::string_view& view)
{
  //  std::ifstream i("gsoc-2018.json");
#if 0
  nlohmann::json j;
  i >> j;
#else
  auto volatile r = nlohmann::json::parse(view.begin(), view.end()); // data, sz);
#endif
}

void outputMe(nlohmann::json& j)
{
  // write prettified JSON to another file
  std::ofstream o("pretty.json");
  o << std::setw(4) << j << std::endl;
}


int
main()
{
  std::ifstream i("citm_catalog.json");
  std::ostringstream sstr;
  sstr << i.rdbuf();
  //  mallopt(M_MMAP_THRESHOLD, 10487560 + 32);
  auto str = sstr.str();
  char * data = str.data();
  size_t sz = str.size();
  auto view = std::string_view(data, sz);
  for (auto i = 0; i < 1000; i++)
  {
#if CHEAPEN
    cheap::cheap<cheap::NONZERO | cheap::SINGLE_THREADED | cheap::DISABLE_FREE> reg;
#endif
    parseMe(view);
  }
  return 0;
}
