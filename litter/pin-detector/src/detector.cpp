#include "pin.H"

#include <fstream>
#include <iostream>
#include <unordered_map>

static KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "detector.out",
                                        "specify output file name");
static std::ofstream OutputFile;

static TLS_KEY TlsKey = INVALID_TLS_KEY;

// For performance benefits, we might want to reserve some space for this
static std::unordered_map<ADDRINT, ADDRINT> addressToObjectSize;

// This is only safe to set (maybe) on a strictly single-threaded program.
#ifndef NO_LOCKS
static PIN_RWMUTEX Lock;
#endif

VOID OnThreadStart(THREADID ThreadId, CONTEXT* Context, INT32 Flags, VOID* _) {
  if (!PIN_SetThreadData(TlsKey, new ADDRINT, ThreadId)) {
    std::cerr << "Failed PIN_SetThreadData..." << std::endl;
    PIN_ExitProcess(1);
  }
}

VOID OnThreadEnd(THREADID ThreadId, const CONTEXT* Context, INT32 Code, VOID* _) {
  delete static_cast<ADDRINT*>(PIN_GetThreadData(TlsKey, ThreadId));
}

VOID OnMallocBefore(THREADID ThreadId, const CONTEXT* Context, ADDRINT Size) {
  ADDRINT* SavedSizePointer = static_cast<ADDRINT*>(PIN_GetThreadData(TlsKey, ThreadId));
  *SavedSizePointer = Size;
}

VOID OnMallocAfter(THREADID ThreadId, ADDRINT Pointer) {
#ifndef NO_LOCKS
  PIN_RWMutexWriteLock(&Lock);
#endif

  ADDRINT SavedSize = *(static_cast<ADDRINT*>(PIN_GetThreadData(TlsKey, ThreadId)));
  // std::cout << "malloc(" << SavedSize << ")" << std::endl;
  for (ADDRINT i = Pointer; i < Pointer + SavedSize; ++i) {
    addressToObjectSize[i] = SavedSize;
  }

#ifndef NO_LOCKS
  PIN_RWMutexUnlock(&Lock);
#endif
}

VOID OnFree(THREADID ThreadId, const CONTEXT* Context, ADDRINT Pointer) {
#ifndef NO_LOCKS
  PIN_RWMutexWriteLock(&Lock);
#endif

  if (addressToObjectSize.find(Pointer) != addressToObjectSize.end()) {
    // std::cout << "free(" << Pointer << ")" << std::endl;
    for (ADDRINT i = Pointer; i < Pointer + addressToObjectSize[Pointer]; ++i) {
      addressToObjectSize.erase(i);
    }
  }

#ifndef NO_LOCKS
  PIN_RWMutexUnlock(&Lock);
#endif
}

VOID OnMemoryRead(THREADID ThreadId, ADDRINT Pointer, UINT32 Size) {
#ifndef NO_LOCKS
  PIN_RWMutexReadLock(&Lock);
#endif

  if (addressToObjectSize.find(Pointer) != addressToObjectSize.end()) {
    // std::cout << "read(" << addressToObjectSize[Pointer] << ")" << std::endl;
  }

#ifndef NO_LOCKS
  PIN_RWMutexUnlock(&Lock);
#endif
}

VOID OnMemoryWrite(THREADID ThreadId, ADDRINT Pointer, UINT32 Size) {
  // No statistics on writes yet...
}

VOID OnProgramEnd(INT32 Code, VOID* _) {}

VOID Instruction(INS Instruction, VOID* _) {
  if (INS_IsMemoryRead(Instruction) && !INS_IsStackRead(Instruction)) {
    INS_InsertCall(Instruction, IPOINT_BEFORE, (AFUNPTR) OnMemoryRead, IARG_THREAD_ID, IARG_MEMORYREAD_EA,
                   IARG_MEMORYREAD_SIZE, IARG_END);
  }

  // // No statistics on writes yet...
  // if (INS_IsMemoryWrite(Instruction) && !INS_IsStackWrite(Instruction)) {
  //   INS_InsertCall(Instruction, IPOINT_BEFORE, (AFUNPTR) OnMemoryWrite, IARG_THREAD_ID, IARG_MEMORYWRITE_EA,
  //                  IARG_MEMORYWRITE_SIZE, IARG_END);
  // }
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

  TlsKey = PIN_CreateThreadDataKey(nullptr);
  if (TlsKey == INVALID_TLS_KEY) {
    std::cerr << "Failed to initialize TLS key..." << std::endl;
    PIN_ExitProcess(1);
  }

#ifndef NO_LOCKS
  if (!PIN_RWMutexInit(&Lock)) {
    std::cerr << "Failed to initialize lock..." << std::endl;
    PIN_ExitProcess(1);
  }
#endif

  IMG_AddInstrumentFunction(Image, nullptr);
  INS_AddInstrumentFunction(Instruction, nullptr);
  PIN_AddThreadStartFunction(OnThreadStart, nullptr);
  PIN_AddThreadFiniFunction(OnThreadEnd, nullptr);
  PIN_AddFiniFunction(OnProgramEnd, nullptr);

  PIN_StartProgram();
}
