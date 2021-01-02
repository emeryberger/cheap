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

static const auto MAX_STACK_LENGTH = 8; // 16384;

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
      // invoke backtrace so it resolves symbols now
      volatile void *dl = dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_GLOBAL);
      void *callstack[4];
      auto frames = backtrace(callstack, 4);
      auto output_file =
          open("cheaper.out", O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
      tprintf::FD = output_file;
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

extern "C" ATTRIBUTE_EXPORT size_t xxmalloc_usable_size(void *ptr) {
  return getTheCustomHeap().getSize(ptr);
}

static std::atomic<bool> firstDone{false};

static void printStack() {
  void *callstack[MAX_STACK_LENGTH];
  char buf[256 * MAX_STACK_LENGTH];
  busy++;
  auto nframes = backtrace(callstack, MAX_STACK_LENGTH);
  char **syms = backtrace_symbols(callstack, nframes);
  busy--;
  char *b = buf;
  // JSON doesn't allow trailing commas at the end,
  // which is stupid, but we have to deal with it.
  for (auto i = 0; i < nframes - 1; i++) {
    tprintf::tprintf("@, ", (uintptr_t)callstack[i]);
  }
  tprintf::tprintf("@", (uintptr_t)callstack[nframes - 1]);
  getTheCustomHeap().free(syms);
}

static void printProlog() {
  if (!firstDone) {
    // First time: start the JSON.
    busy++;
    static InitializeMe init;
    busy--;
    tprintf::tprintf("{ \"trace\" : [\n{\n");
    tprintf::tprintf("  \"action\": \"M\",\n  \"stack\": [");
    firstDone = true;
  } else {
    tprintf::tprintf(",{\n  \"action\": \"M\",\n  \"stack\": [");
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
  size_t real_sz = xxmalloc_usable_size(ptr);
  busy--;
  auto tid = gettid();
  lockme();
  printProlog();
  printStack();
  tprintf::tprintf(
      "],\n  \"size\" : @,\n  \"address\" : @,\n  \"tid\" : @\n}\n", real_sz,
      ptr, tid);
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
  size_t real_sz = xxmalloc_usable_size(ptr);
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

extern "C" ATTRIBUTE_EXPORT void xxfree_sized(void *ptr, size_t sz) {
  xxfree(ptr);
}

extern "C" ATTRIBUTE_EXPORT void *xxmemalign(size_t alignment, size_t sz) {
  if (busy || WeAreOuttaHere::weAreOut) {
    return getTheCustomHeap().memalign(alignment, sz);
  }
  busy++;
  void *ptr = getTheCustomHeap().memalign(alignment, sz);
  auto real_sz = xxmalloc_usable_size(ptr);
  busy--;
  auto tid = gettid();
  lockme();
  printProlog();
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
