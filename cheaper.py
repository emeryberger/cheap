"""cheaper.py: identifies where to apply custom allocators"""

import orjson
import math
import os
import argparse
import sys
import subprocess
from collections import defaultdict
from textwrap import dedent


class Cheaper:

    # For measuring page-level fragmentation
    __pagesize = 4096

    @staticmethod
    def parse():
        usage = dedent(
            """Cheaper: finds where you should use a custom heap.
            """
        )
        parser = argparse.ArgumentParser(
            prog="cheaper",
            description=usage,
            formatter_class=argparse.RawTextHelpFormatter,
            allow_abbrev=False,
        )
        parser.add_argument("--progname", help="path to executable", required=True)
        parser.add_argument(
            "--threshold-mallocs", help="threshold allocations to report", default=100
        )
        parser.add_argument(
            "--threshold-score", help="threshold reporting score", default=0.8
        )
        # parser.add_argument("--skip", help="number of stack frames to skip", default=1)
        parser.add_argument(
            "--depth", help="total number of stack frames to use (from top)", default=5
        )

        args = parser.parse_args()

        if not args.progname:
            parser.print_help()
            sys.exit(-1)

        return args

    def __init__(self, progname, depth):
        with open("cheaper.out", "r") as f:
            file_contents = f.read()
        x = orjson.loads(file_contents)
        trace = x["trace"]
        Cheaper.process_trace(trace, progname, depth)

    @staticmethod
    def utilization(allocs, n):
        """Returns the page-level fragmentation of the sequence of allocs up to the nth item."""
        assert n <= len(allocs)
        pages = defaultdict(int)
        in_use = 0
        mallocs = set()
        for (index, i) in enumerate(allocs):
            if index > n:
                break
            # Ignore objects larger than 1/2 the page size.
            if i["size"] > Cheaper.__pagesize / 2:
                break
            pageno = i["address"] // Cheaper.__pagesize
            if i["action"] == "M":
                mallocs.add(i["address"])
                pages[pageno] += 1
                in_use += i["size"]
            elif i["action"] == "F":
                if i["address"] in mallocs:
                    pages[pageno] -= 1
                    in_use -= i["size"]
                    mallocs.remove(i["address"])
                    if pages[pageno] == 0:
                        del pages[pageno]
        if len(pages) > 0:
            return in_use / (Cheaper.__pagesize * len(pages))
        else:
            return 0

    @staticmethod
    def analyze(allocs, stackstr, progname, depth):
        """Analyze a trace of allocations and frees."""
        if len(allocs) < int(args.threshold_mallocs):
            # Ignore call sites with too few mallocs
            return
        # The set of sizes of allocated objects.
        sizes = set()
        # A histogram of the # of objects allocated of each size.
        size_histogram = defaultdict(int)
        actual_footprint = 0  # mallocs - frees
        peak_footprint = 0  # max actual_footprint
        nofree_footprint = 0  # sum(mallocs)
        # set of all thread ids used for malloc/free
        tids = set()
        # set of all (currently) allocated objects from this site
        mallocs = set()
        utilization = 0
        for (index, i) in enumerate(allocs):
            sizes.add(i["size"])
            size_histogram[i["size"]] += 1
            tids.add(i["tid"])
            if i["action"] == "M":
                # Compute actual footprint (taking into account mallocs and frees).
                actual_footprint += i["size"]
                if actual_footprint > peak_footprint:
                    peak_footprint = actual_footprint
                    # Recompute utilization
                    # FIXME: wasteful, could recompute at end
                    frag = Cheaper.utilization(allocs, index)
                # Compute total 'no-free' memory footprint (excluding frees) This
                # is how much memory would be consumed if we didn't free anything
                # until the end (as with regions/arenas). We use this to compute a
                # "region score" later.
                nofree_footprint += i["size"]
                # Record the malloc so we can check it when freed.
                mallocs.add(i["address"])
            elif i["action"] == "F":
                if i["address"] in mallocs:
                    # Only reclaim memory that we have already allocated
                    # (others are frees to other call sites).
                    actual_footprint -= i["size"]
                    mallocs.remove(i["address"])
                else:
                    pass
                    # print(mallocs)
                    # print(str(i["address"]) + " not found")
        # Compute entropy of sizes
        total = len(allocs)
        normalized_entropy = -sum(
            [
                float(size_histogram[c])
                / total
                * math.log2(float(size_histogram[c]) / total)
                for c in size_histogram
            ]
        ) / len(size_histogram)
        # Compute region_score (0 is worst, 1 is best - for region replacement).
        region_score = 0
        if nofree_footprint != 0:
            region_score = peak_footprint / nofree_footprint
        if region_score >= float(args.threshold_score):
            stk = eval(stackstr)
            print("=====")
            print(stk[0])
            print("-----")
            print("allocations = ", len(allocs))
            print("candidate for region allocation")
            print("* utilization at max = " + str(frag))
            print("* region score = " + str(region_score), "(1 is best)")
            if len(tids) == 1:
                print("* single-threaded (no lock needed)")
            size_list = list(sizes)
            size_list.sort()
            if len(sizes) == 1:
                print("* all the same size (", size_list[0], ")")
            else:
                print(str(len(sizes)) + " different sizes = ", size_list)
                print("size entropy = ", normalized_entropy)
            print("peak footprint = " + str(peak_footprint))
            print("'no free' footprint = " + str(nofree_footprint))

    @staticmethod
    def process_trace(trace, progname, depth):
        stack_series = defaultdict(list)
        stack_info = {}

        # Convert each stack frame into a name and line number
        for i in trace:
            for stkaddr in i["stack"][-depth:]:
                if stkaddr not in stack_info:
                    result = subprocess.run(
                        ["addr2line", hex(stkaddr), "-i", "-f", "-C", "-e", progname],
                        stdout=subprocess.PIPE,
                    )
                    stack_info[stkaddr] = result.stdout.decode("utf-8").strip()

        # Separate each trace by its complete stack signature.
        for i in trace:
            stk = [stack_info[k] for k in i["stack"][-depth:]]  # [skip:depth+skip]]
            stack_series[str(stk)].append(i)

        # Iterate through each call site.
        for k in stack_series:
            Cheaper.analyze(stack_series[k], k, progname, depth)


if __name__ == "__main__":
    args = Cheaper.parse()
    depth = int(args.depth)
    progname = args.progname
    c = Cheaper(progname, depth)
