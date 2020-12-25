# Cheaper

Cheaper is an analysis tool for identifying optimization opportunities via custom allocation.

## Usage

First, you run your program with the Cheaper library, as follows:

    LD_PRELOAD=libcheaper.so ./yourprogram

This generates a file `cheaper.out` in the current directory. That file is a JSON file that contains information used by the `cheaper.py` script.

Note: you may need to install files from `requirements.txt` first:

    pip install -r requirements.txt

Now run Cheaper as follows:

    python3 cheaper.py --progname ./yourprogram

For a complete list of options, type `python3 cheaper.py --help`.

