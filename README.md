# CHeaper

CHeaper: a `malloc`/`new` optimizer

by [Emery Berger](https://emeryberger.com)

# About CHeaper

CHeaper identifies optimizations where a Custom Heap can be used to improve application performance.

CHeaper comprises three components:

1. `libcheaper.so`: a Linux library that produces traces from your C/C++ program's memory management activity
1. `cheaper.py`: a Python script that analyzes the traces and identifies opportunities for placing custom heaps
1. `libcheap.so` and `cheap.h`: the CHeap library and headers, which expose an API for easily supplying an optimized custom heap. 

# Usage

To use CHeaper, first run your program with the CHeaper library, as follows:

    LD_PRELOAD=libcheaper.so ./yourprogram

This generates a file `cheaper.out` in the current directory. That file is a JSON file that contains information used by the `cheaper.py` script.

Note: you may need to install files from `requirements.txt` first:

    pip install -r requirements.txt

Now run CHeaper as follows:

    python3 cheaper.py --progname ./yourprogram

For a complete list of options, type `python3 cheaper.py --help`.

The result will be a sorted list of places where custom heaps can be used. The final line of each is a C++ line of code that can be placed directly in your program. An example might be:

    `cheap::cheap<5436432> reg(cheap::NONZERO | cheap::SINGLE_THREADED);`

Placing this line at the appropriate point in the program will redirect all subsequent allocations and frees to use the generated custom heap. Currently, the only custom heap option is a "region-style" allocator (a.k.a. "arena", "pool", or "monotonic resource").

For an example, see the file `examples/json/testjson.cpp` (line 33).
