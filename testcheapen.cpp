#include <thread>
#include "cheap.h"

#ifndef CHEAPEN
#define CHEAPEN 0
#endif

#ifdef TEST
const auto NUMITERATIONS = 100;
const auto NUMOBJS = 1000;
const int NTHREADS = 1;
#else
const auto NUMITERATIONS = 1000;
const auto NUMOBJS = 100000;
const int NTHREADS = 32;
#endif

const auto OBJSIZE = 16;

void allocWorker()
{
  char buf[NUMOBJS * OBJSIZE];
  char * mem[NUMOBJS];
  for (int i = 0; i < NUMITERATIONS; i++) {
#if CHEAPEN
    //    
    //    region_begin(&buf, NUMOBJS * OBJSIZE);
#endif
    for (int j = 0; j < NUMOBJS; j++) {
      mem[j] = new char[OBJSIZE];
    }
    for (int j = 0; j < NUMOBJS; j++) {
      delete mem[j];
    }
#if CHEAPEN
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
