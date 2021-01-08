"""cheaper.py: identifies where to apply custom allocators"""

try:
    import orjson

    jsonlib = orjson
except BaseException:
    import json

    jsonlib = json
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
    stack_info = {}

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
        )
        parser.add_argument("--progname", help="path to executable", required=True)
        parser.add_argument(
            "--threshold-mallocs", help="threshold allocations to report", default=100
        )
        parser.add_argument(
            "--threshold-score", help="threshold reporting score", default=0.8
        )
        parser.add_argument("--skip", help="number of stack frames to skip", default=0)
        parser.add_argument(
            "--depth", help="total number of stack frames to use (from top)", default=5
        )

        args = parser.parse_args()

        if not args.progname:
            parser.print_help()
            sys.exit(-1)

        return args

    def __init__(self, progname, depth, threshold_mallocs, threshold_score):
        with open("cheaper.out", "r") as f:
            file_contents = f.read()
        x = jsonlib.loads(file_contents)
        trace = x["trace"]
        analyzed = []
        Cheaper.resolve_addresses(trace, progname, depth)
        for d in range(1, depth):
            a = Cheaper.process_trace(
                trace, progname, d, threshold_mallocs, threshold_score
            )
            analyzed += a
        # Sort in reverse order by region score * number of allocations * stack length
        analyzed = sorted(
            analyzed,
            key=lambda a: a["region_score"] * a["allocs"] * len(a["stack"]),
            reverse=True,
        )
        for item in analyzed:
            for stk in item["stack"]:
                print(stk)
            print("-----")
            print("region score = ", item["region_score"])
            print("peak footprint = ", item["peak_footprint"])
            print("region footprint = ", item["nofree_footprint"])
            print("number of allocs = ", item["allocs"])
            print("size taken = ", item["size_taken"])
            print("all aligned = ", item["all_aligned"])
            print("sizes = ", item["sizes"])
            print("threads = ", item["threads"])
            print("=====")

    @staticmethod
    def resolve_addresses(trace, progname, depth):
        """Convert each stack frame into a name and line number"""
        platform = sys.platform
        for i in trace:
            for stkaddr in i["stack"][-depth:]:
                if stkaddr not in Cheaper.stack_info:
                    if platform == "darwin":
                        result = subprocess.run(
                            ["atos", "-o", progname, hex(stkaddr)],
                            stdout=subprocess.PIPE,
                        )
                        print(result.stdout.decode("utf-8").strip())
                    else:
                        result = subprocess.run(
                            ["addr2line", hex(stkaddr), "-C", "-e", progname],
                            stdout=subprocess.PIPE,
                        )
                    Cheaper.stack_info[stkaddr] = result.stdout.decode("utf-8").strip()
                    if "??" in Cheaper.stack_info[stkaddr]:
                        Cheaper.stack_info[stkaddr] = "BAD"

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
                continue
            pageno = Cheaper.__pagesize * (i["address"] // Cheaper.__pagesize)
            if i["action"] == "M":
                mallocs.add(i["address"])
                pages[pageno] += 1
                in_use += i["size"]
            elif i["action"] == "F":
                in_use -= i["size"]
                if i["address"] in mallocs:
                    mallocs.remove(i["address"])
                pages[pageno] -= 1
                if pages[pageno] == 0:
                    del pages[pageno]
        if len(pages) > 0:
            return in_use / (Cheaper.__pagesize * len(pages))
        else:
            return 0

    @staticmethod
    def analyze(allocs, stackstr, progname, depth, threshold_mallocs, threshold_score):
        """Analyze a trace of allocations and frees."""
        analyzed_list = []
        if len(allocs) < int(threshold_mallocs):
            # Ignore call sites with too few mallocs
            return analyzed_list
        # The set of sizes of allocated objects.
        sizes = set()
        # A histogram of the # of objects allocated of each size.
        size_histogram = defaultdict(int)
        actual_footprint = 0  # mallocs - frees
        peak_footprint = 0  # max actual_footprint
        peak_footprint_index = 0  # index of alloc w/max footprint
        nofree_footprint = 0  # sum(mallocs)
        # set of all thread ids used for malloc/free
        tids = set()
        # set of all (currently) allocated objects from this site
        mallocs = set()
        num_allocs = 0
        utilization = 0
        size_taken = False  # true iff size was invoked
        all_aligned = True  # true iff all requests were properly aligned
        for (index, i) in enumerate(allocs):
            sizes.add(i["size"])
            size_histogram[i["size"]] += 1
            tids.add(i["tid"])
            if i["action"] == "S":
                size_taken = True
                continue
            elif i["action"] == "M":
                if i["reqsize"] == 0 or i["reqsize"] % 8 != 0:
                    if all_aligned:
                        print("first reqsize not aligned: " + str(i["reqsize"]))
                    all_aligned = False
                num_allocs += 1
                # Compute actual footprint (taking into account mallocs and frees).
                actual_footprint += i["size"]
                if actual_footprint > peak_footprint:
                    peak_footprint = actual_footprint
                    peak_footprint_index = index
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
        # Recompute utilization
        # frag = Cheaper.utilization(allocs, peak_footprint_index)
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
        if region_score >= float(threshold_score):
            stk = eval(stackstr)
            output = {
                "stack": stk,
                "allocs": num_allocs,
                "region_score": region_score,
                "threads": tids,
                "sizes": sizes,
                "size_entropy": normalized_entropy,
                "peak_footprint": peak_footprint,
                "nofree_footprint": nofree_footprint,
                "size_taken": size_taken,
                "all_aligned": all_aligned,
            }
            analyzed_list.append(output)
        return analyzed_list

    @staticmethod
    def process_trace(trace, progname, depth, threshold_mallocs, threshold_score):
        stack_series = defaultdict(list)
        # Separate each trace by its complete stack signature.
        for i in trace:
            stk = [
                Cheaper.stack_info[k] for k in i["stack"][-depth:]
            ]  # [skip:depth+skip]]
            stk = list(filter(lambda s: s != "BAD", stk))
            if len(stk) == 0:
                continue
            stkstr = str(stk)
            stack_series[stkstr].append(i)

        # Iterate through each call site.
        analyzed = []
        for k in stack_series:
            analyzed += Cheaper.analyze(
                stack_series[k], k, progname, depth, threshold_mallocs, threshold_score
            )
        return analyzed


if __name__ == "__main__":
    args = Cheaper.parse()
    depth = int(args.depth)
    progname = args.progname
    threshold_mallocs = int(args.threshold_mallocs)
    threshold_score = float(args.threshold_score)
    c = Cheaper(progname, depth, threshold_mallocs, threshold_score)
