# Cheap

Cheap: a `malloc`/`new` optimizer

by [Emery Berger](https://emeryberger.com)

# About Cheap

Cheap is a system that makes it easy to improve the performance of
memory-intensive C/C++ applications. Cheap works by replacing memory
allocation operations with a custom heap that can substantially
improve performance. Unlike past approaches, Cheap requires almost no
programming effort. For example, by adding _just one line of code_
(plus an `#include`), we were able to speed up the Boost JSON library
by 30-40%.

# How it works

The Cheap library (`libcheap.so`) and its header `cheap.h` let you use
a custom heap with little effort. One line of code creates a custom
heap with certain characteristics and temporarily redirects _all_
memory allocation calls (`malloc`, `free`, `new`, `delete`) to that
heap until the heap goes out of scope.

To use Cheap, you include the file `cheap.h`, and then insert a line of code creating a custom heap. For example:

    cheap::cheap<cheap::NONZERO | cheap::SINGLE_THREADED | cheap::DISABLE_FREE> r;

The options for the heap are passed as a series of flags, each separated by `|`.

Current options for the custom heap are the following:

* `cheap::NONZERO` -- no requests for 0 bytes
* `cheap::ALIGNED` -- all size requests are suitably aligned
* `cheap::SINGLE_THREADED` -- all allocations and frees are by the same thread
* `cheap::SIZE_TAKEN` -- need to track object sizes for `realloc` or `malloc_usable_size`
* `cheap::SAME_SIZE` -- all object requests are the same size; pass the size as the second argument to the constructor
* `cheap::DISABLE_FREE` -- turns `free` calls into no-ops

Once you place this line at the appropriate point in your program, it
will redirect all subsequent allocations and frees to use the
generated custom heap. _Using `cheap::DISABLE_FREE` gives you the effect of a "region-style" allocator (a.k.a. "arena", "pool", or "monotonic
resource"); otherwise, you get a customized freelist implementation._

Once this object goes out of scope ([RAII-style](https://en.cppreference.com/w/cpp/language/raii), like 
[`std::lock_guard`](https://en.cppreference.com/w/cpp/thread/lock_guard)), the program reverts to ordinary behavior, using the
system-supplied memory allocator, and the custom heap's memory is
reclaimed.

## Placing a custom heap

Sometimes, placing a custom heap is straightforward, but it's nice to
have help. We provide a tool called `Cheaper` that finds places in
your program where a custom heap could potentially improve
performance, and which generates the line of code to insert.

### Collecting a trace with `libcheaper`

First, run your program with the Cheaper library, as follows:

    LD_PRELOAD=libcheaper.so ./yourprogram

This generates a file `cheaper.out` in the current directory. That file is a JSON file that contains information used by the `cheaper.py` script. NOTE: you need to compile your program with several flags to make sure Cheaper can get enough information to work. For example:

    clang++ -g -fno-inline-functions -O0 yourprogram.cpp -o yourprogram

### Analyze the trace with `cheaper.py`

You may need to install files from `requirements.txt` the first time you
run Cheaper:

    pip install -r requirements.txt

Now run Cheaper as follows:

    python3 cheaper.py --progname ./yourprogram

For a complete list of options, type `python3 cheaper.py --help`.

Cheaper will produce output, in ranked order from most to least
promising, corresponding to places in the code where you should insert
the `cheap` declaration. It outputs the line of code, which you can
then copy and paste directly into your program.

