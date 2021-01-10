#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#if !defined(CHEAPEN)
#define CHEAPEN 0
#endif

#if CHEAPEN
#include "cheap.h"
#endif

#include "json.hpp"

void parseMe()
{
  nlohmann::json j;
  std::ifstream i("gsoc-2018.json");
  i >> j;
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
  for (auto i = 0; i < 100; i++)
  {
#if CHEAPEN
    cheap::cheap<5436432> reg(cheap::NONZERO | cheap::SINGLE_THREADED);
#endif
    parseMe();
  }
  return 0;
}
