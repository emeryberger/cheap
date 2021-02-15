#include <heaplayers.h>

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

#if defined(__APPLE__)
#include "macinterpose.h"
#endif

#if defined(__APPLE__)
#define LOCAL_PREFIX(x) xx##x
#else
#define LOCAL_PREFIX(x) x
#endif

static const auto MAX_STACK_LENGTH = 16; // 16384;

#define gettid() (pthread_self())

class ParentHeap : public NextHeap {};

class CustomHeapType : public ParentHeap {
public:
  void lock() {}
  void unlock() {}
};

#if defined(__APPLE__)
#include <mach/mach_init.h>
#include <sys/sysctl.h>
#include <mach/mach_vm.h>
#include <mach-o/getsect.h>
#include <stdio.h>
#include <mach-o/dyld.h>
#include <string.h>

void * getBaseAddress() {
    mach_port_name_t task;
    vm_map_offset_t vmoffset;
    vm_map_size_t vmsize;
    uint32_t nesting_depth = 0;
    struct vm_region_submap_info_64 vbr;
    mach_msg_type_number_t vbrcount = 16;
    kern_return_t kr;

    task = current_task();
    if ((kr = mach_vm_region_recurse(task, &vmoffset, &vmsize,
                 &nesting_depth,
                 (vm_region_recurse_info_t)&vbr,
                 &vbrcount)) != KERN_SUCCESS) 
    {
        printf("FAIL");
    }
    return (void *) vmoffset;
}

uint64_t StaticBaseAddress()
{
    const struct segment_command_64* command = getsegbyname("__TEXT");
    uint64_t addr = command->vmaddr;
    return addr;
}

intptr_t ImageSlide()
{
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return -1;
    for (uint32_t i = 0; i < _dyld_image_count(); i++)
    {
        if (strcmp(_dyld_get_image_name(i), path) == 0)
            return _dyld_get_image_vmaddr_slide(i);
    }
    return 0;
}
#endif

void * baseAddress = 0x0;

class InitializeMe {
public:
  InitializeMe() {
    if (!_isInitialized) {
      // invoke backtrace so it resolves symbols now
      dlopen("libgcc_s.so.1", RTLD_NOW | RTLD_GLOBAL);
      void *callstack[4];
      backtrace(callstack, 4);
      unlink(output_filename);
      auto output_file =
          open(output_filename, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
#if defined(__APPLE__)
      baseAddress = reinterpret_cast<void *>(StaticBaseAddress());
      tprintf::tprintf("base address = @\n", baseAddress);
      tprintf::tprintf("slide = @\n", (void *) ImageSlide());
      //      baseAddress = getBaseAddress();
      //      tprintf::tprintf("base address = @\n", baseAddress);
#endif
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

static std::atomic<bool> firstDone{false};

static void printStack() {
  void *callstack[MAX_STACK_LENGTH];
  busy++;
  auto nframes = backtrace(callstack, MAX_STACK_LENGTH);
#if 0 // defined(__APPLE__)
  char **strs = backtrace_symbols(callstack, nframes);
  for (auto i = 0; i < nframes; ++i) {
    printf("%s\n", strs[i]);
  }
#endif
  busy--;
  // JSON doesn't allow trailing commas at the end,
  // which is stupid, but we have to deal with it.
  for (auto i = 0; i < nframes - 1; i++) {
    tprintf::tprintf("@, ", (uintptr_t)callstack[i] - (uintptr_t) baseAddress);
  }
  tprintf::tprintf("@", (uintptr_t)callstack[nframes - 1] - (uintptr_t) baseAddress);
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
