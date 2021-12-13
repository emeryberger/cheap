#ifndef CHEAP_BACKTRACE_HPP
#define CHEAP_BACKTRACE_HPP

#include <backtrace.h>
#include <cxxabi.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>

// Small utility file I wrote while testing libbacktrace with Intel Pin.
namespace Backtrace {
    namespace {
        static backtrace_state* state = nullptr;
    }

    struct StackFrame {
        std::string function;
        std::string filename;
        int lineno;
    };

    void initialize(char* filename) {
        if (state == nullptr) {
            const auto onError = [](void*, const char* msg, int errnum) {
                std::cerr << "Error #" << errnum << " while setting up backtrace. Message: " << msg << std::endl;
            };

            state = backtrace_create_state(filename, true, onError, nullptr);
        }
    }

    std::vector<StackFrame> getBacktrace(int skip) {
        // If we have not initialized the utility yet, we return an empty stack trace.
        if (state == nullptr) {
            return std::vector<StackFrame>();
        }

        const auto onError = [](void*, const char* msg, int errnum) {
	  std::cerr << "Error #" << errnum << " while getting backtrace. Message: " << msg << std::endl;
        };

        const auto onStackFrame = [](void* data, uintptr_t pc, const char* filename, int lineno, const char* function) {
            auto* backtrace = static_cast<std::vector<StackFrame>*>(data);

            StackFrame frame;

            // If we have no information, we skip this frame.
            if (!function && !filename) {
                return 0;
            }

            if (!function) {
                frame.function = "[UNKNOWN]";
            } else {
                int status;
                char* demangled_function = abi::__cxa_demangle(function, nullptr, nullptr, &status);
		if (demangled_function) {
		  // std::cerr << "DEMANGLED = " << demangled_function << std::endl;
		} else {
		  Dl_info info;
		  int status = dladdr((void *) pc, &info);
		  if (status) {
		    filename = info.dli_sname;
		    //		    std::cerr << "C FUNCTION NAME = " << info.dli_sname << std::endl;
		  }
		}
                if (status == 0) {
                    frame.function = std::string(demangled_function);
                    static decltype(::free)* free = (decltype(::free)*) dlsym(RTLD_DEFAULT, "free");
                    (*free)(demangled_function);
                } else {
                    frame.function = std::string(function);
                }
            }

            frame.filename = (!filename) ? "[UNKNOWN]" : std::string(filename);
            frame.lineno = lineno;

            backtrace->insert(backtrace->begin(), frame);

            return 0;
        };

        std::vector<StackFrame> backtrace;
        backtrace_full(state, skip, onStackFrame, onError, &backtrace);
        return backtrace;
    }

    std::vector<StackFrame> getBacktrace() {
        return getBacktrace(0);
    }
} // namespace Backtrace

#endif // CHEAP_BACKTRACE_HPP
