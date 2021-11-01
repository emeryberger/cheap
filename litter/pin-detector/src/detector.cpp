#include "pin.H"

#include <fstream>
#include <iostream>
#include <unordered_map>

struct ThreadData {
  ADDRINT Size;
};

struct CacheData {
  size_t BinIndex;
};

static KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "detector.out",
                                        "specify output file name");
static std::ofstream OutputFile;

static TLS_KEY TlsKey = INVALID_TLS_KEY;
static std::unordered_map<ADDRINT, CacheData> Cache;
static std::vector<unsigned long long int> Bins(sizeof(intptr_t) * 8);

// NO_LOCKS is only (maybe) safe to enable in strictly single-threaded programs.
#ifndef NO_LOCKS
static PIN_RWMUTEX Lock;

// Written for convenience. Easy to abuse so be careful.
class PIN_RWMutexReadGuard {
public:
  PIN_RWMutexReadGuard(PIN_RWMUTEX* Lock) : _Lock(Lock) { PIN_RWMutexReadLock(_Lock); }

  ~PIN_RWMutexReadGuard() { PIN_RWMutexUnlock(_Lock); }

private:
  PIN_RWMUTEX* _Lock;
};

class PIN_RWMutexWriteGuard {
public:
  PIN_RWMutexWriteGuard(PIN_RWMUTEX* Lock) : _Lock(Lock) { PIN_RWMutexWriteLock(_Lock); }

  ~PIN_RWMutexWriteGuard() { PIN_RWMutexUnlock(_Lock); }

private:
  PIN_RWMUTEX* _Lock;
};
#endif

VOID OnThreadStart(THREADID ThreadId, CONTEXT* Context, INT32 Flags, VOID* _) {
  if (!PIN_SetThreadData(TlsKey, new ThreadData, ThreadId)) {
    std::cerr << "Failed PIN_SetThreadData..." << std::endl;
    PIN_ExitProcess(1);
  }
}

VOID OnThreadEnd(THREADID ThreadId, const CONTEXT* Context, INT32 Code, VOID* _) {
  delete static_cast<ThreadData*>(PIN_GetThreadData(TlsKey, ThreadId));
}

VOID OnMallocBefore(THREADID ThreadId, const CONTEXT* Context, ADDRINT Size) {
  ThreadData* TData = static_cast<ThreadData*>(PIN_GetThreadData(TlsKey, ThreadId));
  TData->Size = Size;
}

VOID OnMallocAfter(THREADID ThreadId, ADDRINT Pointer) {
  ThreadData* TData = static_cast<ThreadData*>(PIN_GetThreadData(TlsKey, ThreadId));

  size_t BinIndex = 0;
  while (BinIndex++ < Bins.size()) {
    if (TData->Size >> BinIndex == 0) {
      break;
    }
  }

  CacheData CData{BinIndex};

#ifndef NO_LOCKS
  PIN_RWMutexWriteGuard _(&Lock);
#endif
  Cache[Pointer] = CData;
}

VOID OnFree(THREADID ThreadId, const CONTEXT* Context, ADDRINT Pointer) {
#ifndef NO_LOCKS
  PIN_RWMutexWriteGuard _(&Lock);
#endif
  Cache.erase(Pointer);
}

VOID OnMemoryRead(THREADID ThreadId, ADDRINT Pointer, UINT32 Size) {
#ifndef NO_LOCKS
  PIN_RWMutexReadGuard _(&Lock);
#endif

  if (Cache.find(Pointer) != Cache.end()) {
    ++Bins[Cache[Pointer].BinIndex];
  }
}

VOID OnMemoryWrite(THREADID ThreadId, ADDRINT Pointer, UINT32 Size) {
  // No statistics on writes yet...
}

VOID OnProgramEnd(INT32 Code, VOID* _) {
#ifndef NO_LOCKS
  PIN_RWMutexFini(&Lock);
#endif

  size_t MaxI = 0;
  for (size_t i = 1; i < Bins.size(); ++i) {
    if (Bins[i] > 0) {
      MaxI = i;
    }
  }

  OutputFile << "{ \"Bins\": [ " << Bins[0];
  for (size_t i = 1; i <= MaxI; ++i) {
    OutputFile << ", " << Bins[i];
  }
  OutputFile << "] }" << std::endl;
}

VOID Instruction(INS Instruction, VOID* _) {
  if (INS_IsMemoryRead(Instruction) && !INS_IsStackRead(Instruction)) {
    INS_InsertCall(Instruction, IPOINT_BEFORE, (AFUNPTR) OnMemoryRead, IARG_THREAD_ID, IARG_MEMORYREAD_EA,
                   IARG_MEMORYREAD_SIZE, IARG_END);
  }

  if (INS_IsMemoryWrite(Instruction) && !INS_IsStackWrite(Instruction)) {
    INS_InsertCall(Instruction, IPOINT_BEFORE, (AFUNPTR) OnMemoryWrite, IARG_THREAD_ID, IARG_MEMORYWRITE_EA,
                   IARG_MEMORYWRITE_SIZE, IARG_END);
  }
}

VOID Image(IMG Image, VOID* _) {
  RTN Routine;

  Routine = RTN_FindByName(Image, "malloc");
  if (RTN_Valid(Routine)) {
    RTN_Open(Routine);
    RTN_InsertCall(Routine, IPOINT_BEFORE, (AFUNPTR) OnMallocBefore, IARG_THREAD_ID, IARG_CONST_CONTEXT,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
    RTN_InsertCall(Routine, IPOINT_AFTER, (AFUNPTR) OnMallocAfter, IARG_THREAD_ID, IARG_FUNCRET_EXITPOINT_VALUE,
                   IARG_END);
    RTN_Close(Routine);
  }

  Routine = RTN_FindByName(Image, "free");
  if (RTN_Valid(Routine)) {
    RTN_Open(Routine);
    RTN_InsertCall(Routine, IPOINT_BEFORE, (AFUNPTR) OnFree, IARG_THREAD_ID, IARG_CONST_CONTEXT,
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_END);
    RTN_Close(Routine);
  }
}

INT32 Usage() {
  std::cerr << "" << std::endl;
  std::cerr << KNOB_BASE::StringKnobSummary() << std::endl;
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  PIN_InitSymbols();
  if (PIN_Init(argc, argv)) {
    return Usage();
  }

  OutputFile.open(KnobOutputFile.Value().c_str());

#ifndef NO_LOCKS
  if (!PIN_RWMutexInit(&Lock)) {
    std::cerr << "Failed to initialize lock..." << std::endl;
    PIN_ExitProcess(1);
  }
#endif

  TlsKey = PIN_CreateThreadDataKey(NULL);
  if (TlsKey == INVALID_TLS_KEY) {
    std::cerr << "Failed to initialize TLS key..." << std::endl;
    PIN_ExitProcess(1);
  }

  IMG_AddInstrumentFunction(Image, nullptr);
  INS_AddInstrumentFunction(Instruction, nullptr);
  PIN_AddThreadStartFunction(OnThreadStart, nullptr);
  PIN_AddThreadFiniFunction(OnThreadEnd, nullptr);
  PIN_AddFiniFunction(OnProgramEnd, nullptr);

  PIN_StartProgram();
}
