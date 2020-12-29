#include <heaplayers.h>

#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.hpp"
#include "nextheap.hpp"
#include "tprintf.h"

#if defined(__APPLE__)
#include "macinterpose.h"
#endif

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

const auto MAX_STACK_LENGTH = 8; // 16384;

#define gettid() (pthread_self())

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
#if 1
      // invoke backtrace so it resolves symbols now
#if 1 // defined(__linux__)
      volatile void *dl = dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_GLOBAL);
#endif
      void *callstack[4];
      auto frames = backtrace(callstack, 4);
#endif
      auto output_file =
          fopen("cheaper.out", "w+"); // O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
      tprintf::FD = fileno(output_file);
      _isInitialized = true;
    }
  }

private:
  std::atomic<bool> _isInitialized{false};
};

static std::atomic<bool> busy{false};

CustomHeapType &getTheCustomHeap() {
  static CustomHeapType thang;
  return thang;
}

// Close the JSON when we exit.
class WeAreOuttaHere {
public:
  ~WeAreOuttaHere() {
    tprintf::tprintf("] }\n");
    weAreOut = true;
  }
  static std::atomic<bool> weAreOut;
};

WeAreOuttaHere bahbye;
std::atomic<bool> WeAreOuttaHere::weAreOut{false};

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void *ptr) {
  return getTheCustomHeap().getSize(ptr);
}

static std::atomic<bool> firstDone{false};
static std::atomic_flag entryLock = ATOMIC_FLAG_INIT;

static void printStack() {
  void *callstack[MAX_STACK_LENGTH];
  char buf[256 * MAX_STACK_LENGTH];
  busy = true;
  auto nframes = backtrace(callstack, MAX_STACK_LENGTH);
  char **syms = backtrace_symbols(callstack, nframes);
  busy = false;
  char *b = buf;
  // JSON doesn't allow trailing commas at the end,
  // which is stupid, but we have to deal with it.
  for (auto i = 0; i < nframes - 1; i++) {
    tprintf::tprintf("@, ", (uintptr_t)callstack[i]);
  }
  tprintf::tprintf("@", (uintptr_t)callstack[nframes - 1]);
  getTheCustomHeap().free(syms);
}

extern "C" ATTRIBUTE_EXPORT void *xxmalloc(size_t sz) {
  // Prevent loop due to internal call by backtrace_symbols
  // and omit any records once we have "ended" to prevent
  // corrupting the JSON output.
  if (busy || WeAreOuttaHere::weAreOut) {
    return getTheCustomHeap().malloc(sz);
  }
  while (entryLock.test_and_set()) {}
  if (!firstDone) {
    // First time: start the JSON.
    busy = true;
    static InitializeMe init;
    busy = false;
#if 0
    // Get the executable name.
    char fname[256];
    // Linux only
    int fd = open("/proc/self/comm", O_RDONLY);
    read(fd, fname, 256);
    // Strip off trailing carriage return.
    fname[strlen(fname)-1] = '\0';
    while (entryLock.test_and_set()) {}
    tprintf::tprintf("{ \"executable\" : \"@\",\n", fname);
    tprintf::tprintf("  \"trace\" : [\n{\n");
#else
    tprintf::tprintf("{ \"trace\" : [\n{\n");
#endif
    tprintf::tprintf("  \"action\": \"M\",\n  \"stack\": [");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"M\",\n  \"stack\": [");
  }
  printStack();
  void *ptr = getTheCustomHeap().malloc(sz);
  auto tid = gettid();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n",
      xxmalloc_usable_size(ptr), ptr, tid);
  entryLock.clear();
  return ptr;
}

extern "C" ATTRIBUTE_EXPORT void xxfree(void *ptr) {
  if (busy || WeAreOuttaHere::weAreOut) {
    getTheCustomHeap().free(ptr);
    return;
  }
  while (entryLock.test_and_set()) {}
  if (!firstDone) {
    tprintf::tprintf("[\n{\n  \"action\": \"F\",\n  \"stack\": [");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"F\",\n  \"stack\": [");
  }
  printStack();
  auto tid = gettid();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n",
      xxmalloc_usable_size(ptr), ptr, tid);
  entryLock.clear();
  getTheCustomHeap().free(ptr);
}

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void *ptr, size_t sz) {
  xxfree(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmemalign(size_t alignment, size_t sz) {
  return getTheCustomHeap().memalign(alignment, sz);
}

extern "C" ATTRIBUTE_EXPORT void xxmalloc_lock() { getTheCustomHeap().lock(); }

extern "C" ATTRIBUTE_EXPORT void xxmalloc_unlock() {
  getTheCustomHeap().unlock();
}
