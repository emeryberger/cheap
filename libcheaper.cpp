#include <heaplayers.h>

#include <execinfo.h>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

const char output_filename[] = "cheaper.out";

#include "common.hpp"
#include "nextheap.hpp"

#include "printf.h"

auto output_file = open(output_filename, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);

// For use by the replacement printf routines (see
// https://github.com/mpaland/printf)
extern "C" void _putchar(char ch) { ::write(output_file, (void *)&ch, 1); }

#include "Backtrace.hpp"

#if defined(__APPLE__)
#include "macinterpose.h"
#endif

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach/mach_init.h>
#include <mach/mach_vm.h>
#include <string.h>
#include <sys/sysctl.h>
#endif

#define gettid() (pthread_self())

class ParentHeap : public NextHeap {};

class CustomHeapType : public ParentHeap {
public:
  void lock() {}
  void unlock() {}
};

CustomHeapType& getTheCustomHeap() {
  static CustomHeapType thang;
  return thang;
}

static std::atomic<long> samples{0};
static std::atomic<int> busy{0};
#define USE_LOCKS 0

#if USE_LOCKS
static std::atomic_flag entryLock = ATOMIC_FLAG_INIT;
static void lockme() {
  while (entryLock.test_and_set()) {
  }
}
static void unlockme() { entryLock.clear(); }
#else
static void lockme() {}
static void unlockme() {}
#endif

// Open the JSON file before we start recording events, close it when we are done.
class Initialization {
public:
  Initialization() {
    ++busy;
    // tprintf::FD = output_file;
    // tprintf::tprintf("{ \"trace\": [");
    printf_("{ \"trace\": [");

    Backtrace::initialize(nullptr);
    --busy;

    isReadyToSample = true;
  }

  ~Initialization() {
    isReadyToSample = false;

    lockme();
    printf_("\n]}");
    // tprintf::tprintf("\n]}");
    fsync(output_file);
    unlockme();
  }

  static std::atomic<bool> isReadyToSample;
};

static Initialization _;
std::atomic<bool> Initialization::isReadyToSample{false};

static void printStack() {
  busy++;

  // We don't skip any frames.
  // Extra stack frames will get filtered out by the utility since libbacktrace won't have access to Cheap's debug info.
  for (const auto& frame : Backtrace::getBacktrace()) {
    char buf[1024];
    snprintf_(buf, 1024, "[%s] %s:%d", frame.function.c_str(), frame.filename.c_str(), frame.lineno);
    printf_("\"%s\", ", buf);
    //    tprintf::tprintf("\"@\", ", buf);
  }

  // Now print a bogus stack with no commas, just to make JSON processing happy.
  printf_("\"CHEAPERBAD\""); // Filtered out by cheaper.py
  // tprintf::tprintf("\"CHEAPERBAD\""); // Filtered out by cheaper.py

  busy--;
}

static void printProlog(char action) {
  if (samples != 0) {
    printf_(",");
    //    tprintf::tprintf(",");
  }

  printf_("\n\t{ \"action\": \"%c\", \"stack\": [", action);
  //  tprintf::tprintf("\n\t{ \"action\": \"@\", \"stack\": [", action);
}

extern "C" ATTRIBUTE_EXPORT void* xxmalloc(size_t sz) {
  // The number of bytes that need to be processed until the next sample.
  // This count is per-thread.
  static thread_local long long int sampling_count = 0;

  // Prevent loop due to internal call by backtrace_symbols
  // and omit any records once we have "ended" to prevent
  // corrupting the JSON output.
  if (busy || !Initialization::isReadyToSample) {
    return getTheCustomHeap().malloc(sz);
  }

  busy++;
  void* ptr = getTheCustomHeap().malloc(sz);
  size_t real_sz = getTheCustomHeap().getSize(ptr);
  busy--;

  auto tid = gettid();
  lockme();
  printProlog('M');
  printStack();
  printf_("], \"size\": %lu, \"reqsize\": %lu, \"address\": %p, \"tid\": %d }", real_sz, sz, ptr, tid);
  // tprintf::tprintf("], \"size\": @, \"reqsize\": @, \"address\": @, \"tid\": @ }", real_sz, sz, ptr, tid);
  unlockme();

  ++samples;

  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void* ptr) {
  if (busy || !Initialization::isReadyToSample) {
    getTheCustomHeap().free(ptr);
    return;
  }

  busy++;
  // Note: this info is redundant (it will already be in the trace) and may be
  // removed.
  size_t real_sz = getTheCustomHeap().getSize(ptr);
  getTheCustomHeap().free(ptr);
  busy--;

  auto tid = gettid();
  lockme();
  printProlog('F');
  printStack();
  printf_("], \"size\": %lu, \"address\": %p, \"tid\": %d }", real_sz, ptr, tid);
  // tprintf::tprintf("], \"size\": @, \"address\": @, \"tid\": @ }", real_sz, ptr, tid);
  unlockme();
  ++samples;
}

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void* ptr) {
  auto real_sz = getTheCustomHeap().getSize(ptr);
  if (busy || !Initialization::isReadyToSample) {
    return real_sz;
  }
  auto tid = gettid();
  lockme();
  printProlog('S');
  printStack();
  printf_("], \"size\": %lu, \"address\": %p, \"tid\": %d }", real_sz, ptr, tid);
  // tprintf::tprintf("], \"size\": @, \"address\": @, \"tid\": @ }", real_sz, ptr, tid);
  unlockme();
  ++samples;

  return real_sz;
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void* ptr, size_t) { xxfree(ptr); }

extern "C" ATTRIBUTE_EXPORT void* xxmemalign(size_t alignment, size_t sz) {
  if (busy || !Initialization::isReadyToSample) {
    return getTheCustomHeap().memalign(alignment, sz);
  }
  busy++;
  void* ptr = getTheCustomHeap().memalign(alignment, sz);
  auto real_sz = getTheCustomHeap().getSize(ptr);
  busy--;
  auto tid = gettid();
  lockme();
  printProlog('A');
  printStack();
  printf_("], \"size\": %lu, \"address\": %p, \"tid\": %d }", real_sz, ptr, tid);
  // tprintf::tprintf("], \"size\": @, \"address\": @, \"tid\": @ }", real_sz, ptr, tid);
  unlockme();
  ++samples;

  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() { getTheCustomHeap().unlock(); }

#if !defined(__APPLE__)
#include "gnuwrapper.cpp"
#endif
