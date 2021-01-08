#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#if defined(CHEAPEN)
#include "cheapen.h"
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
#if defined(CHEAPEN)
    cheapen_region<256 * 1048576, false, true, false> reg;
#endif
    parseMe();
  }
  return 0;
}
