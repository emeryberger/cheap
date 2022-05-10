#include <dlfcn.h>
#include <random>

#define ATTRIBUTE_EXPORT __attribute__((visibility("default")))

// On average, we turn every 200th free into a noop.
#define NOOP_RATE 200

static std::random_device Generator;
static std::geometric_distribution<int> Distribution(1. / NOOP_RATE);
static thread_local int SamplingCount = Distribution(Generator);

extern "C" ATTRIBUTE_EXPORT void free(void* Pointer) noexcept {
    static decltype(::free)* Free = (decltype(::free)*) dlsym(RTLD_NEXT, "free");

    --SamplingCount;

    if (SamplingCount > 0) {
        (*Free)(Pointer);
    } else {
        SamplingCount = Distribution(Generator);
    }
}