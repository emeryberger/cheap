# CHeaper

CHeaper: a `malloc`/`new` optimizer

by [Emery Berger](https://emeryberger.com)

# About CHeaper

CHeaper identifies optimizations where a Custom Heap can be used to improve application performance.

CHeaper consists of two parts:
1. `libcheaper.so`, a Linux library that produces traces from your C/C++ program's memory management activity, and
1. `cheaper.py`, a Python script that analyzes the traces.

# Usage

First, you run your program with the CHeaper library, as follows:

    LD_PRELOAD=libcheaper.so ./yourprogram

This generates a file `cheaper.out` in the current directory. That file is a JSON file that contains information used by the `cheaper.py` script.

Note: you may need to install files from `requirements.txt` first:

    pip install -r requirements.txt

Now run CHeaper as follows:

    python3 cheaper.py --progname ./yourprogram

For a complete list of options, type `python3 cheaper.py --help`.

