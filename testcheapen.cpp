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
const int NTHREADS = 1; // 32;
#endif

const auto OBJSIZE = 16;

void doWork(volatile char** mem)
{
  for (int j = 0; j < NUMOBJS; j++) {
    mem[j] = new char[OBJSIZE];
        volatile char ch = mem[j][0];
    //    delete [] mem[j];
  }
#if 1
  for (int j = 0; j < NUMOBJS; j++) {
    delete [] mem[j];
  }
#endif
}

void allocWorker()
{
  char buf[NUMOBJS * OBJSIZE];
  volatile char * mem[NUMOBJS];
  for (int i = 0; i < NUMITERATIONS; i++) {
#if CHEAPEN
    cheap::cheap<cheap::ALIGNED | cheap::NONZERO | cheap::SAME_SIZE | cheap::SINGLE_THREADED | cheap::DISABLE_FREE> reg(24);
    //    cheap::cheap<cheap::ALIGNED | cheap::NONZERO | cheap::SAME_SIZE | cheap::SINGLE_THREADED> reg(24);
#endif
    doWork(mem);
  }
}

#define TEST 1 // FIXME

int main()
{
#if TEST
  allocWorker();
#else
  std::thread * t[NTHREADS];
  for (auto i = 0; i < NTHREADS; i++) {
    t[i] = new std::thread(allocWorker);
  }
  for (auto i = 0; i < NTHREADS; i++) {
    t[i]->join();
  }
#endif
  return 0;
}
