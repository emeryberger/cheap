#include <heaplayers.h>

#include <iostream>
#include <execinfo.h>
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
#include "tprintf.h"

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
#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>
#include <string.h>
#endif


#define gettid() (pthread_self())

void* baseAddress = 0x0;

class ParentHeap : public NextHeap {};

class CustomHeapType : public ParentHeap {
public:
  void lock() {}
  void unlock() {}
};

class InitializeMe {
public:
  InitializeMe() {
    if (!_isInitialized) {
      unlink(output_filename);
      auto output_file =
          open(output_filename, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
      tprintf::FD = output_file;

      // We cannot know the name of the executable, therefore we pass nullptr and hope libbacktrace can figure it out.
      // It actually does a pretty good job. See https://github.com/ianlancetaylor/libbacktrace/blob/master/fileline.c.
      Backtrace::initialize(nullptr);
      _isInitialized = true;
    }
  }

private:
  std::atomic<bool> _isInitialized{false};
};

static std::atomic<int> busy{0};

CustomHeapType &getTheCustomHeap() {
  static CustomHeapType thang;
  return thang;
}

static std::atomic_flag entryLock = ATOMIC_FLAG_INIT;

#define USE_LOCKS 0

#if USE_LOCKS
static void lockme() {
  while (entryLock.test_and_set()) {
  }
}
static void unlockme() { entryLock.clear(); }
#else
static void lockme() {}
static void unlockme() {}
#endif

// Close the JSON when we exit.
class WeAreOuttaHere {
public:
  ~WeAreOuttaHere() {
    weAreOut = true;
    lockme();
    tprintf::tprintf("] }\n");
    fsync(tprintf::FD);
    unlockme();
  }
  static std::atomic<bool> weAreOut;
};

static WeAreOuttaHere bahbye;
std::atomic<bool> WeAreOuttaHere::weAreOut{false};

static std::atomic<bool> firstDone{false};

static void printStack() {
  busy++;

  // We don't skip any frames.
  // Extra stack frames will get filtered out by the utility since libbacktrace won't have access to Cheap's debug info.
  for (const auto& frame : Backtrace::getBacktrace()) {
      char buf[1024];
      snprintf(buf, 1024, "[%s] %s:%d", frame.function.c_str(), frame.filename.c_str(), frame.lineno);
      tprintf::tprintf("\"@\", ", buf);
    }

    // Now print a bogus stack with no commas, just to make JSON processing happy.
    tprintf::tprintf("\"CHEAPERBAD\""); // Filtered out by cheaper.py

  busy--;
}

static bool printProlog(char action) {
  if (!firstDone) {
    // First time: start the JSON.
    busy++;
    static InitializeMe init;
    busy--;
    tprintf::tprintf("{ \"trace\" : [\n{\n");
    tprintf::tprintf("  \"action\": \"@\",\n  \"stack\": [", action);
    firstDone = true;
    return true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"@\",\n  \"stack\": [", action);
    return false;
  }
}

extern "C" ATTRIBUTE_EXPORT void *xxmalloc(size_t sz) {
  // Prevent loop due to internal call by backtrace_symbols
  // and omit any records once we have "ended" to prevent
  // corrupting the JSON output.
  if (busy || WeAreOuttaHere::weAreOut) {
    return getTheCustomHeap().malloc(sz);
  }
  busy++;
  void *ptr = getTheCustomHeap().malloc(sz);
  size_t real_sz = getTheCustomHeap().getSize(ptr);
  busy--;
  auto tid = gettid();
  lockme();
  printProlog('M');
  printStack();
  tprintf::tprintf("],\n \"size\" : @,\n \"reqsize\" : @,\n \"address\" : @,\n "
                   "\"tid\" : @\n}\n",
                   real_sz, sz, ptr, tid);
  unlockme();
  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void *ptr) {
  if (busy || WeAreOuttaHere::weAreOut) {
    getTheCustomHeap().free(ptr);
    return;
  }
  auto tid = gettid();
  busy++;
  // Note: this info is redundant (it will already be in the trace) and may be
  // removed.
  size_t real_sz = getTheCustomHeap().getSize(ptr);
  getTheCustomHeap().free(ptr);
  busy--;
  lockme();
  if (!firstDone) {
    tprintf::tprintf("[\n{\n  \"action\": \"F\",\n  \"stack\": [");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"F\",\n  \"stack\": [");
  }
  printStack();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n", real_sz,
      ptr, tid);
  unlockme();
}

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void *ptr) {
  auto real_sz = getTheCustomHeap().getSize(ptr);
  if (busy || WeAreOuttaHere::weAreOut) {
    return real_sz;
  }
  auto tid = gettid();
  lockme();
  if (!firstDone) {
    tprintf::tprintf("[\n{\n  \"action\": \"S\",\n  \"stack\": [");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"S\",\n  \"stack\": [");
  }
  printStack();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n", real_sz,
      ptr, tid);
  unlockme();
  return real_sz;
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void *ptr, size_t) {
  xxfree(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmemalign(size_t alignment, size_t sz) {
  if (busy || WeAreOuttaHere::weAreOut) {
    return getTheCustomHeap().memalign(alignment, sz);
  }
  busy++;
  void *ptr = getTheCustomHeap().memalign(alignment, sz);
  auto real_sz = getTheCustomHeap().getSize(ptr);
  busy--;
  auto tid = gettid();
  lockme();
  printProlog('A');
  printStack();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n", real_sz,
      ptr, tid);
  unlockme();
  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}

#if !defined(__APPLE__)
#include "gnuwrapper.cpp"
#endif
