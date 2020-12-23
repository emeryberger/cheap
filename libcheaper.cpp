#include <heaplayers.h>

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.hpp"

#if defined(__APPLE__)
extern void malloc_printf(const char *fmt, ...);
#include "macinterpose.h"
#include "tprintf.h"
#endif

#include "nextheap.hpp"

class ParentHeap: public NextHeap {};

class CustomHeapType : public ParentHeap {
public:
  void lock() {}
  void unlock() {}
};

//typedef NextHeap CustomHeapType;

class InitializeMe {
public:
  InitializeMe()
  {
#if 1
    // invoke backtrace so it resolves symbols now
#if 1 // defined(__linux__)
    volatile void * dl = dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_GLOBAL);
#endif
    void * callstack[4];
    auto frames = backtrace(callstack, 4);
#endif
    //    isInitialized = true;
  }
};

static std::atomic<bool> busy { false };

#if 0

static CustomHeapType thang;
#define getTheCustomHeap() thang

#else

CustomHeapType& getTheCustomHeap() {
  static CustomHeapType thang;
  return thang;
}

#endif

class WeAreOuttaHere {
public:
  ~WeAreOuttaHere()
  {
    tprintf::tprintf("] }\n");
    weAreOut = true;
  }
  static bool weAreOut;
};

WeAreOuttaHere bahbye;
bool WeAreOuttaHere::weAreOut { false };

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

const auto MAX_STACK_LENGTH = 4; // 16384;

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void * ptr) {
  return getTheCustomHeap().getSize(ptr); // TODO FIXME adjust for ptr offset?
}

static std::atomic<bool> firstDone { false };

extern "C" ATTRIBUTE_EXPORT void * xxmalloc(size_t sz) {
  if (busy || WeAreOuttaHere::weAreOut) {
    return getTheCustomHeap().malloc(sz); // ::malloc(sz);
  }
  if (!firstDone) {
    busy = true;
    static InitializeMe init;
    busy = false;
    tprintf::tprintf("{ \"trace\" : [\n{\n");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n");
  }
  tprintf::tprintf("  \"action\": \"M\",\n  \"stack\": [");
  void * ptr = getTheCustomHeap().malloc(sz);
  void * callstack[MAX_STACK_LENGTH];
  busy = true;
  auto nframes = backtrace(callstack, MAX_STACK_LENGTH);
  char ** syms = backtrace_symbols(callstack, nframes);
  busy = false;
  for (auto i = 1; i < nframes; i++) {
    //    tprintf::tprintf("\"@\"", (const char *) syms[i]);
    //    char buf[255];
    //    sprintf(buf, "0x%lX", (uintptr_t) callstack[i]);
    // tprintf::tprintf("\"@\"", (const char *) buf);
    if (i < nframes - 1) {
      tprintf::tprintf("@, ", (uintptr_t) callstack[i]);
    } else {
      tprintf::tprintf("@", (uintptr_t) callstack[i]);
    }
  }
  getTheCustomHeap().free(syms);
  tprintf::tprintf("],\n  \"size\" : @,\n  \"address\" : @\n}\n", sz, ptr);
  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void * ptr) {
  if (busy || WeAreOuttaHere::weAreOut) {
    getTheCustomHeap().free(ptr); // ::free(ptr);
    return;
  }
  if (!firstDone) {
    tprintf::tprintf("[\n");
    tprintf::tprintf("{\n");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n");
  }    
  tprintf::tprintf("  \"action\": \"F\",\n  \"stack\": [");
  void * callstack[MAX_STACK_LENGTH];
  busy = true;
  auto nframes = backtrace(callstack, MAX_STACK_LENGTH);
  auto syms = backtrace_symbols(callstack, nframes);
  busy = false;
  uintptr_t stack_hash = 0;
  for (auto i = 1; i < nframes; i++) {
    if (i < nframes - 1) {
      tprintf::tprintf("@, ", (uintptr_t) callstack[i]);
    } else {
      tprintf::tprintf("@", (uintptr_t) callstack[i]);
    }
    stack_hash ^= (uintptr_t) callstack[i];
  }
  getTheCustomHeap().free(syms);
  //  ::free(syms);
  tprintf::tprintf("],\n  \"size\" : @,\n  \"address\" : @\n}\n", xxmalloc_usable_size(ptr), ptr);
  getTheCustomHeap().free(ptr);
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void * ptr, size_t sz) {
  // TODO FIXME maybe make a sized-free version?
  xxfree(ptr); // getTheCustomHeap().free(ptr);
}

extern "C" ATTRIBUTE_EXPORT void * xxmemalign(size_t alignment, size_t sz) {
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() {
  getTheCustomHeap().lock();
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}
