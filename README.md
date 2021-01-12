# CHeaper

CHeaper: a `malloc`/`new` optimizer

by [Emery Berger](https://emeryberger.com)

# About CHeaper

CHeaper makes it easy to replace memory allocation operations with a
custom heap that can substantially improve performance. For example,
with _just one line of code_, CHeap speeds up the Boost JSON library
by 30-40%.

# How it works

The CHeap library (`libcheap.so`) and its header `cheap.h` let you use
a custom heap with little effort. One line of code creates a custom
heap and temporarily redirects _all_ memory allocation calls
(`malloc`, `free`, `new`, `delete`) to that heap until the heap goes
out of scope.

To use CHeap, you include the file `cheap.h`, and then insert a line of code creating a custom heap. For example:

    cheap::cheap reg(cheap::NONZERO | cheap::SINGLE_THREADED);

The options for the heap are passed as a series of flags, each separated by `|`.

Current options for the custom heap are the following:

* `cheap::NONZERO` -- no requests for 0 bytes
* `cheap::ALIGNED` -- all size requests are suitably aligned
* `cheap::SINGLE_THREADED` -- all allocations and frees are by the same thread
* `cheap::SIZE_TAKEN` -- need to track object sizes for `realloc` or `malloc_usable_size`

Once you place this line at the appropriate point in your program, it
will redirect all subsequent allocations and frees to use the
generated custom heap. (Note: currently, the only custom heap option
is a "region-style" allocator (a.k.a. "arena", "pool", or "monotonic
resource").) Once this object goes out of scope (RAII-style, like a
`guard`), the program reverts to ordinary behavior, using the
system-supplied memory allocator, and the custom heap's memory is
reclaimed.

## Placing a custom heap

Sometimes, placing a custom heap is straightforward, but it's nice to
have help. We provide a tool called `CHeaper` that finds places in
your program where a custom heap could potentially improve
performance, and which generates the line of code to insert.

### Collecting a trace with `libcheaper`

First, run your program with the CHeaper library, as follows:

    LD_PRELOAD=libcheaper.so ./yourprogram

This generates a file `cheaper.out` in the current directory. That file is a JSON file that contains information used by the `cheaper.py` script.

Note: you may need to install files from `requirements.txt` first:

    pip install -r requirements.txt

### Analyze the trace with `cheaper.py`

Now run CHeaper as follows:

    python3 cheaper.py --progname ./yourprogram

For a complete list of options, type `python3 cheaper.py --help`.

CHeaper will produce output, in ranked order from most to least
promising, corresponding to places in the code where you should insert
the `cheap` declaration. It outputs the line of code, which you can
then copy and paste directly into your program.

