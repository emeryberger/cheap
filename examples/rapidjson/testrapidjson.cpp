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

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

void parseMe(const char * s, size_t sz)
{
  {
#if CHEAPEN
    cheap::cheap<cheap::ALIGNED | cheap::NONZERO | cheap::SINGLE_THREADED | cheap::SIZE_TAKEN | cheap::DISABLE_FREE> reg; //  | cheap::SIZE_TAKEN
#endif
    using namespace rapidjson;
#if 1
    CrtAllocator alloc;
    GenericDocument<UTF8<>, CrtAllocator> d(&alloc);
#else
    //Document d;
    GenericDocument<UTF8<>> d;
#endif
    d.Parse(s, sz);
  }
}

int main()
{
  //   std::ifstream i("gsoc-2018.json");
  std::ifstream i("citm_catalog.json");
  std::ostringstream sstr;
  sstr << i.rdbuf();
  //  mallopt(M_MMAP_THRESHOLD, 10487560 + 32);
  for (auto i = 0; i < 1000; i++)
  {
    parseMe(sstr.str().data(), sstr.str().size());
  }
  return 0;
}
