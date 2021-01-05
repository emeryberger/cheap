#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "cheapen.h"

#include "json.hpp"

nlohmann::json j;

void parseMe()
{
  std::ifstream i("gsoc-2018.json");
  //  nlohmann::json j;
  i >> j;
}

void outputMe()
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
    cheapen_region<256 * 1048576, true, false> reg;
    parseMe();
  }
  _exit(0);
  return 0;
}
