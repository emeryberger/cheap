#include <thread>
#include "cheapen.h"

#ifndef USE_REGIONS
#define USE_REGIONS 0
#endif

const auto NUMITERATIONS = 1000;
const auto NUMOBJS = 100000;
const auto OBJSIZE = 16;
const int NTHREADS = 32;

void allocWorker()
{
  char buf[NUMOBJS * OBJSIZE];
  char * mem[NUMOBJS];
  for (int i = 0; i < NUMITERATIONS; i++) {
#if USE_REGIONS
    region_begin(&buf, NUMOBJS * OBJSIZE);
#endif
    for (int j = 0; j < NUMOBJS; j++) {
      mem[j] = new char[OBJSIZE];
    }
    for (int j = 0; j < NUMOBJS; j++) {
      delete mem[j];
    }
#if USE_REGIONS
    region_end();
#endif
  }
}

int main()
{
  std::thread * t[NTHREADS];
  for (auto i = 0; i < NTHREADS; i++) {
    t[i] = new std::thread(allocWorker);
  }
  for (auto i = 0; i < NTHREADS; i++) {
    t[i]->join();
  }
  return 0;
}
